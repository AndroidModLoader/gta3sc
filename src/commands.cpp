#include <stdinc.h>
#include "commands.hpp"
#include "program.hpp"
#include "symtable.hpp"

Commands::Commands(insensitive_map<std::string, Command>&& commands_,
                   insensitive_map<std::string, std::vector<const Command*>>&& alternators_,
                   transparent_map<std::string, EntityType>&& entities_,
                   transparent_map<std::string, shared_ptr<Enum>>&& enums_)

    : commands(std::move(commands_)), alternators(std::move(alternators_)),
      enums(std::move(enums_)), entities(std::move(entities_))
{
    auto it_defaultmodel = this->enums.find("DEFAULTMODEL");
    auto it_model       = this->enums.find("MODEL");
    auto it_scriptstream= this->enums.find("SCRIPTSTREAM");

    assert(it_model != this->enums.end());
    assert(it_defaultmodel != this->enums.end());
    assert(it_scriptstream != this->enums.end());

    this->enum_models = it_model->second;
    this->enum_defaultmodels = it_defaultmodel->second;
    this->enum_scriptstream = it_scriptstream->second;

    for(auto& pair : this->commands)
    {
        if(pair.second.id)
            this->commands_by_id.emplace(*pair.second.id, &pair.second);
    }

    this->set_progress_total            = find_command("SET_PROGRESS_TOTAL");
    this->set_total_number_of_missions  = find_command("SET_TOTAL_NUMBER_OF_MISSIONS");
    this->set_collectable1_total        = find_command("SET_COLLECTABLE1_TOTAL");
    this->switch_start                  = find_command("SWITCH_START");
    this->switch_continued              = find_command("SWITCH_CONTINUED");
    this->gosub_file                    = find_command("GOSUB_FILE");
    this->launch_mission                = find_command("LAUNCH_MISSION");
    this->load_and_launch_mission_internal = find_command("LOAD_AND_LAUNCH_MISSION_INTERNAL");
    this->start_new_script              = find_command("START_NEW_SCRIPT");
    this->start_new_streamed_script     = find_command("START_NEW_STREAMED_SCRIPT");
    this->terminate_this_script         = find_command("TERMINATE_THIS_SCRIPT");
    this->script_name                   = find_command("SCRIPT_NAME");
    this->return_                        = find_command("RETURN");
    this->cleo_call                     = find_command("CLEO_CALL");
    this->cleo_return                   = find_command("CLEO_RETURN");
    this->terminate_this_custom_script  = find_command("TERMINATE_THIS_CUSTOM_SCRIPT");
    this->goto_                          = find_command("GOTO");
    this->goto_if_false                 = find_command("GOTO_IF_FALSE");
    this->andor                         = find_command("ANDOR");
    this->save_string_to_debug_file     = find_command("SAVE_STRING_TO_DEBUG_FILE");
    this->skip_cutscene_start           = find_command("SKIP_CUTSCENE_START");
    this->skip_cutscene_end             = find_command("SKIP_CUTSCENE_END");
    this->skip_cutscene_start_internal  = find_command("SKIP_CUTSCENE_START_INTERNAL");
    this->set                           = find_alternator("SET");
    this->cset                          = find_alternator("CSET");
    this->add_thing_to_thing            = find_alternator("ADD_THING_TO_THING");
    this->sub_thing_from_thing          = find_alternator("SUB_THING_FROM_THING");
    this->mult_thing_by_thing           = find_alternator("MULT_THING_BY_THING");
    this->div_thing_by_thing            = find_alternator("DIV_THING_BY_THING");
    this->add_thing_to_thing_timed      = find_alternator("ADD_THING_TO_THING_TIMED");
    this->sub_thing_from_thing_timed    = find_alternator("SUB_THING_FROM_THING_TIMED");
    this->is_thing_equal_to_thing       = find_alternator("IS_THING_EQUAL_TO_THING");
    this->is_thing_greater_than_thing   = find_alternator("IS_THING_GREATER_THAN_THING");
    this->is_thing_greater_or_equal_to_thing = find_alternator("IS_THING_GREATER_OR_EQUAL_TO_THING");
    this->register_streamed_script_internal = find_command("REGISTER_STREAMED_SCRIPT_INTERNAL");
    this->require                       = find_command("REQUIRE");
}

optional<std::string> Commands::find_command_name(uint16_t id, bool never_fail) const
{
    // TODO speed up this search

    for(auto& pair : this->commands)
    {
        if(pair.second.id == id)
            return pair.first;
    }

    if(never_fail)
    {
        char buffer[sizeof("COMMAND_")-1 + 4 + 1];
        snprintf(buffer, sizeof(buffer), "COMMAND_%.4X", id);
        return std::string(std::begin(buffer), std::end(buffer) - 1);
    }

    return nullopt;
}

optional<std::string> Commands::find_entity_name(EntityType type) const
{
    if(type == 0)
        return std::string("NONE");

    for(auto& pair : this->entities)
    {
        if(pair.second == type)
            return pair.first;
    }

    return nullopt;
}

void Commands::add_default_models(const insensitive_map<std::string, uint32_t>& default_models)
{
    for(auto& model_pair : default_models)
    {
        this->enum_defaultmodels->values.emplace(model_pair);
    }
}

optional<int32_t> Commands::find_constant(const string_view& value, bool context_free_only) const
{
    // TODO mayyybe speed up this? we didn't profile or anything.
    for(auto& enum_pair : enums)
    {
        if(enum_pair.second->is_global == context_free_only)
        {
            if(auto opt = enum_pair.second->find(value))
                return opt;
        }
    }
    return nullopt;
}

optional<int32_t> Commands::find_constant_all(const string_view& value) const
{
    // See https://github.com/thelink2012/gta3sc/issues/60
    if(auto opt = enum_defaultmodels->find(value))
        return opt;

    // TODO mayyybe speed up this? we didn't profile or anything.
    for(auto& enum_pair : enums)
    {
        if(&enum_pair.second == &enum_defaultmodels)
            continue;
        if(auto opt = enum_pair.second->find(value))
            return opt;
    }
    return nullopt;
}

optional<int32_t> Commands::find_constant_for_arg(const string_view& value, const Command::Arg& arg) const
{
    if(arg.type == ArgType::Constant)
    {
        if(auto opt_const = this->find_constant_all(value))
            return opt_const;
    }
    else
    {
        if(auto opt_const = arg.find_constant(value)) // constants stricly related to this Arg
            return opt_const;
    }

    // If the enum that the argument accepts is MODEL, and the above didn't find a match,
    // also try on the DEFAULTMODEL enum.
    if(arg.uses_enum(this->enum_models))
    {
        if(auto opt_const = enum_defaultmodels->find(value))
            return opt_const;
    }

    if(arg.type != ArgType::Constant)
    {
        if(auto opt_const = this->find_constant(value, true)) // global constants
            return opt_const;
    }

    return nullopt;
}

///////////////////////////////////////////////////////////////////////////////

using MatchFailure = Commands::MatchFailure;

struct TagVar
{
    string_view ident;
};

static auto maybe_var_identifier(const string_view& ident, const Command::Arg& arginfo) -> optional<std::pair<string_view, bool>>
{
    if(arginfo.type != ArgType::TextLabel && arginfo.type != ArgType::TextLabel16 && arginfo.type != ArgType::String)
    {
        if(arginfo.type == ArgType::Param && arginfo.allow_constant && arginfo.allow_text_label)
        {
            if(ident.size() && ident.front() == '$')
                return std::make_pair(ident.substr(1), true);
        }
        return std::make_pair(ident, false);
    }
    else // TextLabel
    {
        if(arginfo.allow_global_var || arginfo.allow_local_var)
        {
            if(!arginfo.allow_constant)
                return std::make_pair(ident, false);
            else if(ident.size() && ident.front() == '$')
                return std::make_pair(ident.substr(1), true);
        }
        return nullopt;
    }
}

template<typename T>
static auto args_from_tree(const SyntaxTree& cmdnode) -> T
{
    T output;
    output.reserve(cmdnode.child_count() - 1);

    for(auto it = std::next(cmdnode.begin()); it != cmdnode.end(); ++it)
        output.emplace_back(it->get());

    return output;
}

static auto hint_from(optional<const SyntaxTree&> cmdnode) -> shared_ptr<const SyntaxTree>
{
    return cmdnode? cmdnode->shared_from_this() : nullptr;
}

static auto hint_from(optional<const SyntaxTree&> cmdnode, const Commands::MatchArgumentList::value_type& arg) -> shared_ptr<const SyntaxTree>
{
    if(is<const SyntaxTree*>(arg))
        return get<const SyntaxTree*>(arg)->shared_from_this();
    return hint_from(cmdnode);
}

static auto match_arg(const Commands& commands, const shared_ptr<const SyntaxTree>& hint,
                      int32_t arg, const Command::Arg& arginfo,
                      const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) -> expected<const Command::Arg*, MatchFailure>
{
    if(arginfo.type == ArgType::Integer || arginfo.type == ArgType::Constant || arginfo.type == ArgType::Param)
        return &arginfo;
    return make_unexpected(MatchFailure{ hint, MatchFailure::ExpectedInt });
}

static auto match_arg(const Commands& commands, const shared_ptr<const SyntaxTree>& hint,
                      float arg, const Command::Arg& arginfo,
                      const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) -> expected<const Command::Arg*, MatchFailure>
{
    if(arginfo.type == ArgType::Float || arginfo.type == ArgType::Param)
        return &arginfo;
    return make_unexpected(MatchFailure{ hint, MatchFailure::ExpectedFloat });
}

static auto match_arg(const Commands& commands, const shared_ptr<const SyntaxTree>& hint,
                      const TagVar& arg, const Command::Arg& arginfo,
                      const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) -> expected<const Command::Arg*, MatchFailure>
{
    auto var_matches = [](const shared_ptr<Var>& var, const Command::Arg& arginfo) -> bool
    {
        switch(var->type)
        {
            case VarType::Int:
                return (arginfo.type == ArgType::Integer
                     || arginfo.type == ArgType::Constant 
                     || arginfo.type == ArgType::Param 
                     || (arginfo.type == ArgType::String && arginfo.allow_pointer)
                     || (arginfo.type == ArgType::TextLabel && arginfo.allow_pointer)
                     || (arginfo.type == ArgType::TextLabel16 && arginfo.allow_pointer));
            case VarType::Float:
                return (arginfo.type == ArgType::Float || arginfo.type == ArgType::Param);
            case VarType::TextLabel:
                return (arginfo.type == ArgType::TextLabel
                    ||  arginfo.type == ArgType::String
                    || (arginfo.type == ArgType::Param && arginfo.allow_text_label));
            case VarType::TextLabel16:
                return (arginfo.type == ArgType::TextLabel16
                    ||  arginfo.type == ArgType::String
                    || (arginfo.type == ArgType::Param && arginfo.allow_text_label));
            default:
                Unreachable();
        }
    };

    if(!Miss2Identifier::is_identifier(arg.ident))
        return make_unexpected(MatchFailure{ hint, MatchFailure::InvalidIdentifier });

    if(auto var_ident = maybe_var_identifier(arg.ident, arginfo))
    {
        auto opt_token = Miss2Identifier::match(var_ident->first);
        if(!opt_token)
        {
            switch(opt_token.error())
            {
                case Miss2Identifier::InvalidIdentifier:return make_unexpected(MatchFailure{ hint, MatchFailure::InvalidIdentifier });
                case Miss2Identifier::NestingOfArrays:  return make_unexpected(MatchFailure{ hint, MatchFailure::IdentifierIndexNesting });
                case Miss2Identifier::NegativeIndex:    return make_unexpected(MatchFailure{ hint, MatchFailure::IdentifierIndexNegative });
                case Miss2Identifier::OutOfRange:       return make_unexpected(MatchFailure{ hint, MatchFailure::IdentifierIndexOutOfRange });
                default:                                Unreachable();
            }
        }

        auto& token = *opt_token;
        if(auto opt_var = symtable.find_var(token.identifier, scope_ptr))
        {
            if(token.index != nullopt && !is<size_t>(*token.index))
            {
                auto& index = get<string_view>(*token.index);
                if(auto opt_varidx = symtable.find_var(index, scope_ptr))
                {
                    if((*opt_varidx)->type != VarType::Int)
                        return make_unexpected(MatchFailure{ hint, MatchFailure::VariableIndexNotInt });
                    if((*opt_varidx)->count)
                        return make_unexpected(MatchFailure{ hint, MatchFailure::VariableIndexIsArray });
                }
                else if(commands.find_constant_all(index) == nullopt) // TODO -pedantic error
                {
                    return make_unexpected(MatchFailure{ hint, MatchFailure::VariableIndexNotVar });
                }
            }
            else if(token.index == nullopt && (*opt_var)->count != nullopt)
            {
                return make_unexpected(MatchFailure{ hint, MatchFailure::ExpectedVarIndex });
            }

            auto& var = *opt_var;
            if(!(arginfo.allow_global_var && var->global) && !(arginfo.allow_local_var && !var->global))
            {
                return make_unexpected(MatchFailure{ hint, MatchFailure::VariableKindNotAllowed });
            }

            if(!var_matches(var, arginfo))
                return make_unexpected(MatchFailure{ hint, MatchFailure::VariableTypeMismatch });

            return &arginfo;
        }
        else if(var_ident->second) // var not found, but required to find one?
        {
            return make_unexpected(MatchFailure{ hint, MatchFailure::ExpectedVar });
        }
    }

    return make_unexpected(MatchFailure{ hint, MatchFailure::NoSuchVar });
}

static auto match_arg(const Commands& commands, const shared_ptr<const SyntaxTree>& hint,
                      string_view text, const Command::Arg& arginfo,
                      const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) -> expected<const Command::Arg*, MatchFailure>
{
    switch(arginfo.type)
    {
        case ArgType::Label:
            if(Miss2Identifier::is_identifier(text))
            {
                if(symtable.find_label(text))
                    return &arginfo;
                else
                    return make_unexpected(MatchFailure { hint, MatchFailure::NoSuchLabel });
            }
            else
                return make_unexpected(MatchFailure{ hint, MatchFailure::InvalidIdentifier });

        case ArgType::Constant:
            if(Miss2Identifier::is_identifier(text))
            {
                if(commands.find_constant_all(text))
                    return &arginfo;
                else
                    return make_unexpected(MatchFailure{ hint, MatchFailure::NoSuchConstant });
            }
            else
                return make_unexpected(MatchFailure{ hint, MatchFailure::InvalidIdentifier });

        case ArgType::TextLabel:
        case ArgType::TextLabel16:
        case ArgType::String:
        {
            auto exp_var = match_arg(commands, hint, TagVar { text }, arginfo, symtable, scope_ptr);
            if(exp_var)
                return exp_var;
            else if(exp_var.error().reason == MatchFailure::NoSuchVar && arginfo.allow_constant)
                return &arginfo;
            else if(exp_var.error().reason == MatchFailure::InvalidIdentifier && arginfo.type == ArgType::String && arginfo.allow_constant)
                return &arginfo;
            else
                return exp_var; // error state
        }

        case ArgType::Integer:
        case ArgType::Float:
        case ArgType::Param:
        {
            if(arginfo.allow_constant && arginfo.type == ArgType::Integer)
            {
                if(commands.find_constant_for_arg(text, arginfo))
                    return &arginfo;
            }

            auto exp_var = match_arg(commands, hint, TagVar { text }, arginfo, symtable, scope_ptr);
            if(exp_var || exp_var.error().reason != MatchFailure::NoSuchVar)
                return exp_var;
            else if(arginfo.uses_enum(commands.get_scriptstream_enum()) && symtable.find_streamed_id(text))
                return &arginfo;
            else if(arginfo.uses_enum(commands.get_models_enum())) // allow unknown models
                return &arginfo;
            else if(arginfo.type == ArgType::Param && arginfo.allow_constant && arginfo.allow_text_label)
                return &arginfo;
            else
                return make_unexpected(MatchFailure{ hint, MatchFailure::NoSuchVar });
        }

        default:
            Unreachable();
    }
}

static auto match_arg(const Commands& commands, const shared_ptr<const SyntaxTree>& hint,
                      const SyntaxTree& arg, const Command::Arg& arginfo,
                      const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) -> expected<const Command::Arg*, MatchFailure>
{
    // TODO FIXME this matcher gives wrong error messages
    switch(arg.type())
    {
        case NodeType::Integer:
            return match_arg(commands, hint, 0, arginfo, symtable, scope_ptr);
        case NodeType::Float:
            return match_arg(commands, hint, 0.0f, arginfo, symtable, scope_ptr);
        case NodeType::Text:
            return match_arg(commands, hint, arg.text(), arginfo, symtable, scope_ptr);
        case NodeType::String:
            if(arginfo.type == ArgType::String
            || (arginfo.type == ArgType::Param && arginfo.allow_text_label))
                return &arginfo;
            else
                return make_unexpected(MatchFailure{ hint, MatchFailure::StringLiteralNotAllowed });
        default:
            Unreachable();
    }
}

auto Commands::match(const SyntaxTree& cmdnode, const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) const -> expected<const Command*, MatchFailure>
{
    auto command_name = cmdnode.child(0).text();

    if(auto opt_alternator = this->find_alternator(command_name))
    {
        return this->match(*opt_alternator, cmdnode, symtable, scope_ptr);
    }
    else if(auto opt_command = this->find_command(command_name))
    {
        return this->match(*opt_command, cmdnode, symtable, scope_ptr);
    }
    else
    {
        return make_unexpected(MatchFailure { cmdnode.shared_from_this(), MatchFailure::NoCommandMatch });
    }
}

auto Commands::match(const Alternator& alternator, const SyntaxTree& cmdnode,
                     const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) const -> expected<const Command*, MatchFailure>
{
    return this->match(alternator, cmdnode, args_from_tree<MatchArgumentList>(cmdnode), symtable, scope_ptr);
}

auto Commands::match(const Command& command, const SyntaxTree& cmdnode,
                     const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) const -> expected<const Command*, MatchFailure>
{
    return this->match(command, cmdnode, args_from_tree<MatchArgumentList>(cmdnode), symtable, scope_ptr);
}

auto Commands::match(const Alternator& alternator, optional<const SyntaxTree&> cmdnode, const MatchArgumentList& args,
                     const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) const -> expected<const Command*, MatchFailure>
{
    for(auto& cmd : alternator)
    {
        if(auto opt_command = this->match(*cmd, cmdnode, args, symtable, scope_ptr))
            return opt_command;
    }
    return make_unexpected(MatchFailure { hint_from(cmdnode), MatchFailure::NoAlternativeMatch });
}

auto Commands::match(const Command& command, optional<const SyntaxTree&> cmdnode, const MatchArgumentList& args,
                     const SymTable& symtable, const shared_ptr<Scope>& scope_ptr) const -> expected<const Command*, MatchFailure>
{
    size_t i = 0;
    expected<const Command::Arg*, MatchFailure> exp_arg;
    for(auto it = args.begin(); it != args.end(); ++it, ++i)
    {
        if(auto arginfo = command.arg(i))
        {
            if(is<int32_t>(*it))
                exp_arg = match_arg(*this, hint_from(cmdnode, *it), 0, *arginfo, symtable, scope_ptr);
            else if(is<float>(*it))
                exp_arg = match_arg(*this, hint_from(cmdnode, *it), 0.0f, *arginfo, symtable, scope_ptr);
            else // is<const SyntaxTree*>
                exp_arg = match_arg(*this, hint_from(cmdnode, *it), *get<const SyntaxTree*>(*it), *arginfo, symtable, scope_ptr);

            if(!exp_arg)
            {
                return make_unexpected(std::move(exp_arg.error()));
            }
        }
        else
        {
            return make_unexpected(MatchFailure{ hint_from(cmdnode, *it), MatchFailure::TooManyArgs });
        }
    }

    if(i < command.minimum_args())
        return make_unexpected(MatchFailure{ hint_from(cmdnode), MatchFailure::TooFewArgs });

    return &command;
}

void Commands::annotate(SyntaxTree& cmdnode, const Command& command,
                        const SymTable& symtable, const shared_ptr<Scope>& scope_ptr,
                        Script& script, ProgramContext& program) const
{
    return this->annotate(args_from_tree<AnnotateArgumentList>(cmdnode), command, symtable, scope_ptr, script, program);
}

void Commands::annotate(const AnnotateArgumentList& args, const Command& command,
                        const SymTable& symtable, const shared_ptr<Scope>& scope_ptr,
                        Script& script, ProgramContext& program) const
{
    // Expects all args to match command.args!

    auto find_var = [&](const string_view& value) -> optional<VarAnnotation>
    {
        auto opt_token = Miss2Identifier::match(value);
        if(!opt_token)
            return nullopt;

        auto& token = *opt_token;
        if(auto opt_var = symtable.find_var(token.identifier, scope_ptr))
        {
            using index_type = decltype(ArrayAnnotation::index);
            if(token.index == nullopt)
            {
                return VarAnnotation{ *opt_var, nullopt };
            }
            else if(is<size_t>(*token.index))
            {
                auto& index = get<size_t>(*token.index);
                return VarAnnotation{ *opt_var, index_type(index) };
            }
            else
            {
                auto& index = get<string_view>(*token.index);
                if(auto opt_varidx = symtable.find_var(index, scope_ptr))
                {
                    return VarAnnotation{ *opt_var, index_type(*opt_varidx) };
                }
                else if(auto opt_idx = this->find_constant_all(index))
                {
                    return VarAnnotation{ *opt_var, index_type(*opt_idx) };
                }
            }
        }

        return nullopt;
    };

    // TODO maybe start using VarAnnotation directly instead of converting to shared_ptr<Var>
    // Think of implications of this, such as the `any` type stack size.
    auto annotate_var = [](SyntaxTree& node, const VarAnnotation& annotation)
    {
        Expects(annotation.base != nullptr);
        if(annotation.index)
        {
            if(node.is_annotated())
                Expects(node.maybe_annotation<const ArrayAnnotation&>());
            else
                node.set_annotation(ArrayAnnotation{ annotation.base, *annotation.index });
        }
        else
        {
            if(node.is_annotated())
                Expects(node.maybe_annotation<const shared_ptr<Var>&>());
            else
                node.set_annotation(annotation.base);
        }
    };

    auto annotate_string = [&](SyntaxTree& node, const Command::Arg& arginfo)
    {
        if(node.is_annotated())
            Expects(node.maybe_annotation<const TextLabelAnnotation&>());
        else
        {
            bool preserve_case = (node.type() == NodeType::String && arginfo.preserve_case);

            size_t limit = arginfo.type == ArgType::TextLabel?    7 :
                           arginfo.type == ArgType::TextLabel16?  15 :
                           arginfo.type == ArgType::String? 127 :
                           arginfo.type == ArgType::Param? 127 : Unreachable();

            if(node.text().size() <= limit)
            {
                auto string = node.type() == NodeType::String? remove_quotes(node.text()).to_string() : node.text().to_string();
                node.set_annotation(TextLabelAnnotation{ arginfo.type != ArgType::TextLabel, preserve_case, std::move(string) });
            }
            else
            {
                auto type = (node.type() == NodeType::String? "string" : "identifier");
                program.error(node, "{} is too long, maximum size is {}", type, limit);
            }
        }
    };

    size_t i = 0;
    for(auto it = args.begin(); it != args.end(); ++it, ++i)
    {
        if(is<nullopt_t>(*it))
            continue;

        auto& node = *get<SyntaxTree*>(*it);
        auto& arginfo = *command.arg(i);

        switch(node.type())
        {
            case NodeType::Integer:
            {
                if(node.is_annotated())
                    Expects(node.maybe_annotation<const int32_t&>());
                else
                    node.set_annotation(static_cast<int32_t>(std::stoi(node.text().to_string(), nullptr, 0)));
                break;
            }

            case NodeType::Float:
            {
                if(node.is_annotated())
                    Expects(node.maybe_annotation<const float&>());
                else
                    node.set_annotation(std::stof(node.text().to_string()));
                break;
            }

            case NodeType::String:
            {
                if(arginfo.type == ArgType::String || arginfo.type == ArgType::Param)
                {
                    if(!program.opt.cleo)
                        program.error(node, "string literals on arguments are disallowed [-fcleo]");

                    annotate_string(node, arginfo);
                }
                else
                {
                    // TODO, SAVE_STRING_TO_DEBUG_FILE currently Unreachable() due to special handling of it.
                    Unreachable();
                }
                break;
            }

            case NodeType::Text:
            {
                if(arginfo.type == ArgType::Label)
                {
                    if(node.is_annotated())
                        Expects(node.maybe_annotation<const shared_ptr<Label>&>());
                    else
                        node.set_annotation(symtable.find_label(node.text()).value());
                }
                else if(arginfo.type == ArgType::TextLabel
                     || arginfo.type == ArgType::TextLabel16
                     || arginfo.type == ArgType::String)
                {
                    if(auto opt_match = maybe_var_identifier(node.text(), arginfo))
                    {
                        if(auto opt_var = find_var(opt_match->first))
                        {
                            annotate_var(node, *opt_var);
                            break;
                        }
                    }

                    if(arginfo.allow_constant)
                    {
                        annotate_string(node, arginfo);
                        break;
                    }

                    Unreachable();
                }
                else if(arginfo.type == ArgType::Integer || arginfo.type == ArgType::Float
                     || arginfo.type == ArgType::Constant || arginfo.type == ArgType::Param)
                {
                    if(arginfo.type == ArgType::Integer || arginfo.type == ArgType::Constant)
                    {
                        if(auto opt_const = this->find_constant_for_arg(node.text(), arginfo))
                        {
                            if(node.is_annotated())
                                Expects(node.maybe_annotation<const int32_t&>());
                            else
                                node.set_annotation(*opt_const);
                            break;
                        }
                    }

                    if(arginfo.uses_enum(this->get_scriptstream_enum()))
                    {
                        node.set_annotation(int32_t { symtable.find_streamed_id(node.text()).value() });
                        break;
                    }

                    bool is_model_enum = arginfo.uses_enum(this->get_models_enum());
                    // TODO VC miss2 doesn't allow models and vars with same name?

                    if(!is_model_enum || !program.is_model_from_ide(node.text()))
                    {
                        if(auto opt_match = maybe_var_identifier(node.text(), arginfo))
                        {
                            if(auto opt_var = find_var(opt_match->first))
                            {
                                if(!opt_var->base->is_text_var() || opt_match->second) // if text var, shall begin with $
                                {
                                    annotate_var(node, *opt_var);
                                    break;
                                }
                            }
                        }
                    }

                    if(is_model_enum)
                    {
                        if(node.is_annotated())
                            Expects(node.maybe_annotation<const ModelAnnotation&>());
                        else
                        {
                            auto index = script.add_or_find_model(node.text());
                            assert(index >= 0);
                            node.set_annotation(ModelAnnotation{ script.shared_from_this(), uint32_t(index) });
                        }
                        break;
                    }

                    if(arginfo.type == ArgType::Param && arginfo.allow_text_label)
                    {
                        annotate_string(node, arginfo);
                        break;
                    }
                    
                    Unreachable();
                }
                else
                {
                    Unreachable();
                }
                break;
            }

            default:
                Unreachable();
        }
    }

    Ensures(i >= command.minimum_args());
}

void Commands::MatchFailure::emit(ProgramContext& program)
{
    auto message = this->to_string();
    if(this->context)
        program.error(*this->context, message.c_str());
    else
        program.error(nocontext, message.c_str());
}

std::string Commands::MatchFailure::to_string()
{
    switch(this->reason)
    {
        case NoCommandMatch:            return "unknown command";
        case NoAlternativeMatch:        return "could not match alternative";
        case TooManyArgs:               return "too many arguments";
        case TooFewArgs:                return "too few arguments";
        case BadArgument:               return "bad argument";
        case ExpectedInt:               return "expected integer";
        case ExpectedFloat:             return "expected float";
        case NoSuchLabel:               return "no label with this name";
        case NoSuchConstant:            return "no string constant with this value";
        case NoSuchVar:                 return "no variable with this name";
        case ExpectedVar:               return "expected variable";
        case InvalidIdentifier:         return ::to_string(Miss2Identifier::InvalidIdentifier);
        case IdentifierIndexNesting:    return ::to_string(Miss2Identifier::NestingOfArrays);
        case IdentifierIndexNegative:   return ::to_string(Miss2Identifier::NegativeIndex);
        case IdentifierIndexOutOfRange: return ::to_string(Miss2Identifier::OutOfRange);
        case VariableIndexNotInt:       return "variable in index is not of INT type";
        case VariableIndexNotVar:       return "identifier between brackets is not a variable";
        case VariableIndexIsArray:      return "variable in index is of array type";
        case VariableKindNotAllowed:    return "variable kind (global/local) not allowed for this argument";
        case VariableTypeMismatch:      return "variable type does not match argument type";
        case StringLiteralNotAllowed:   return "STRING literal not allowed here";
        case ExpectedVarIndex:          return "use of array variable without a index";
        default:                        Unreachable();
    }
}
