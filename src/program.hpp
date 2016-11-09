#pragma once
#include "stdinc.h"
#include "parser.hpp"
#include "symtable.hpp"
#include "commands.hpp"

struct tag_nocontext_t {};
constexpr tag_nocontext_t nocontext = {};

// TODO better name?
struct HaltJobException : public std::exception
{
};

struct Options
{
    enum class Lang : uint8_t
    {
        IR2,
        GTA3Script,
    };

    enum class HeaderVersion : uint8_t
    {
        None,
        GTA3,
        GTAVC,
        GTASA,
    };

    /// Boolean flags
    // TODO maybe use bitfields?
    bool headerless = false;
    bool pedantic = false;
    bool guesser = false;
    bool use_half_float = false;
    bool has_text_label_prefix = false;
    bool skip_single_ifs = false;
    bool optimize_zero_floats = false;
    bool entity_tracking = true;
    bool script_name_check = true;
    bool fswitch = false;
    bool allow_break_continue = false;
    bool scope_then_label = false;
    bool farrays = false;
    bool streamed_scripts = false;
    bool text_label_vars = false;
    bool use_local_offsets = false;
    bool skip_cutscene = false;
    bool fsyntax_only = false;
    bool emit_ir2 = false;
    bool linear_sweep = false;
    bool relax_not = false;
    bool output_cleo = false;

    // 8 bit stuff
    HeaderVersion header = HeaderVersion::None;
    optional<uint8_t> cleo;

    // 32 bit stuff
    int32_t            timer_index = 0;
    uint32_t           local_var_limit = 0;
    uint32_t           mission_var_begin = 0;
    optional<uint32_t> mission_var_limit;
    optional<uint32_t> switch_case_limit;
    optional<uint32_t> array_elem_limit;


    // TEnum = CompiledScmHeader::Version or DecompiledScmHeader::Version
    // If this->header is HeaderVersion::None, the behaviour is undefined.
    template<typename TEnum>
    auto get_header() const -> TEnum
    {
        switch(this->header)
        {
            case HeaderVersion::None: Unreachable();
            case HeaderVersion::GTA3: return TEnum::Liberty;
            case HeaderVersion::GTAVC: return TEnum::Miami;
            case HeaderVersion::GTASA: return TEnum::SanAndreas;
            default: Unreachable();
        }
    }

    void define(std::string symbol, std::string value = "1")
    {
        defines.emplace(std::move(symbol), std::move(value));
    }

    void undefine(const string_view& symbol)
    {
        auto it = defines.find(symbol);
        if(it != defines.end()) defines.erase(it);
    }

    bool is_defined(const string_view& symbol) const
    {
        return defines.find(symbol) != defines.end();
    }

private:
    transparent_map<std::string, std::string> defines;
};

template<typename... Args>
inline std::string format_error(const char* type,
                                optional<const TokenStream::TextStream&> stream,
                                const char* filename, uint32_t lineno, uint32_t colno,
                                const char* msg, Args&&... args)
{
    std::string message;
    message.reserve(255);

    if(filename)
    {
        message += filename;
        message.push_back(':');
    }
    else
    {
        message += "gta3sc:";
    }

    if(lineno)
    {
        message += std::to_string(lineno);
        message.push_back(':');
    }

    if(lineno && colno)
    {
        message += std::to_string(colno);
        message.push_back(':');
    }

    if(message.size())
    {
        message.push_back(' ');
    }

    if(type)
    {
        message += type;
        message += ": ";
    }

    message += fmt::format(msg, std::forward<Args>(args)...);

    if(stream && lineno)
    {
        message += fmt::format("\n {}\n {:>{}}", stream->get_line(lineno), "^", colno);
    }

    return message;
}

template<typename... Args>
inline std::string format_error(const char* type, tag_nocontext_t, const char* msg, Args&&... args)
{
    return format_error(type, nullopt, nullptr, 0, 0, msg, std::forward<Args>(args)...);
}

template<typename... Args>
inline std::string format_error(const char* type, const Script& script, const char* msg, Args&&... args)
{
    return format_error(type, nullopt, script.path.generic_u8string().c_str(), 0, 0, msg, std::forward<Args>(args)...);
}

template<typename... Args>
inline std::string format_error(const char* type, const TokenStream::TokenInfo& context, const char* msg, Args&&... args)
{
    if(context.begin == context.end)
    {
        return format_error(type, nullopt, context.stream.stream_name.c_str(), 0, 0,
                            msg, std::forward<Args>(args)...);
    }
    else
    {
        size_t lineno, colno;
        std::tie(lineno, colno) = context.stream.linecol_from_offset(context.begin);
        return format_error(type, context.stream, context.stream.stream_name.c_str(), lineno, colno,
                            msg, std::forward<Args>(args)...);
    }
}

template<typename... Args>
inline std::string format_error(const char* type, const SyntaxTree& context_, const char* msg, Args&&... args)
{
    const SyntaxTree* context = &context_;

    if(!context->has_text())
    {
        auto it = std::find_if(context->begin(), context->end(), [](const shared_ptr<SyntaxTree>& child) {
            return child->has_text();
        });
        context = (it == context->end())? context : it->get();
    }

    if(context->token_stream().use_count() == 0)
    {
        return format_error("internal_error", nocontext, "context->token_stream() == nullptr during format_error");
    }
    else
    {
        auto tstream = context->token_stream().lock();
        return format_error(type, TokenStream::TokenInfo(tstream->text, context->get_token()), msg, std::forward<Args>(args)...);
    }
}

class ProgramContext
{
public:
    const Options opt;          ///< Compiler options / flags.
    const Commands commands;    ///< Commands, Entities and Enums

public:
    explicit ProgramContext(Options opt, Commands commands) :
        opt(std::move(opt)), commands(std::move(commands))
    {
    }

    ProgramContext(const ProgramContext&) = delete;
    ProgramContext(ProgramContext&&) = delete;

    void setup_models(std::map<std::string, uint32_t, iless> default_models,
                      std::map<std::string, uint32_t, iless> level_models)
    {
        this->default_models = std::move(default_models);
        this->level_models   = std::move(level_models);
    }

    bool is_model_from_ide(const string_view& name) const;

    bool has_error() const
    {
        return (this->error_count > 0 || this->fatal_count > 0);
    }

    template<typename Context, typename... Args>
    void error(const Context& context, const char* msg, Args&&... args)
    {
        ++error_count;
        this->puts(format_error("error", context, msg, std::forward<Args>(args)...));

        /* TODO limit error_count
        if(error_count > 100)
            this->fatal_error(nocontext, "too many errors");
        */
    }

    template<typename Context, typename... Args>
    void note(const Context& context, const char* msg, Args&&... args)
    {
        this->puts(format_error("note", context, msg, std::forward<Args>(args)...));
    }

    template<typename Context, typename... Args>
    void warning(const Context& context, const char* msg, Args&&... args)
    {
        ++warn_count;
        this->puts(format_error("warning", context, msg, std::forward<Args>(args)...));
    }

    template<typename Context, typename... Args>
    void fatal_error [[noreturn]] (const Context& context, const char* msg, Args&&... args)
    {
        ++fatal_count;
        this->puts(format_error("fatal error", context, msg, std::forward<Args>(args)...));
        throw HaltJobException();
    }

    void register_errors(uint32_t n)
    {
        // n may be 0
        error_count += n;
    }

    template<typename Context>
    auto supported_or_fatal(Context& context, optional<const Command&> opt, const char* name) -> const Command&
    {
        if(opt == nullopt || !opt->supported)
            this->fatal_error(context, "command '{}' undefined or unsupported", name);
        return *opt;
    }

    template<typename Context>
    auto supported_or_fatal(Context& context, optional<const Commands::Alternator&> opt, const char* name) -> const Commands::Alternator&
    {
        if(opt == nullopt)
            this->fatal_error(context, "alternator '{}' undefined or unsupported", name);
        return *opt;
    }

private:
    static void puts(const std::string& msg)
    {
        std::fprintf(stderr, "%s\n", msg.c_str());
    }

private:
    std::atomic<uint32_t> error_count {0};
    std::atomic<uint32_t> fatal_count {0};
    std::atomic<uint32_t> warn_count  {0};

protected:
    friend struct Commands;

    std::map<std::string, uint32_t, iless> default_models;
    std::map<std::string, uint32_t, iless> level_models;

    // TODO profile whether unordered_map is faster for the models table on current standard lib impls.
    // hmmm we'd need to lowercase the string for unordered_map since it hashes...
};

/// \warning Not thread-safe.
/// \throws ConfigError on failure.
extern void load_ide(const fs::path& filepath, bool is_default_ide, std::map<std::string, uint32_t, iless>& output);

/// \warning Not thread-safe.
/// \throws ConfigError on failure.
extern auto load_dat(const fs::path& filepath, bool is_default_dat) -> std::map<std::string, uint32_t, iless>;
