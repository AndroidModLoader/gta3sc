// RUN: %dis %gta3sc %s --config=gtasa --guesser --cs -fsyntax-only 2>&1 | 
{
VAR_INT x // expected-error {{declaring global variables in custom scripts}}
TERMINATE_THIS_CUSTOM_SCRIPT
}