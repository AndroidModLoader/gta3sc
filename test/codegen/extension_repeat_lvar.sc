// RUN: %gta3sc %s --config=gta3 -emit-ir2 -o - | %FileCheck %s
// RUN: %gta3sc %s --config=gtavc -emit-ir2 -o - | %FileCheck %s
// RUN: %gta3sc %s --config=gtasa --guesser -emit-ir2 -o - | %FileCheck %s
// RUN: %not %gta3sc %s --config=gta3 -emit-ir2 -o - -pedantic-errors
// RUN: %not %gta3sc %s --config=gtavc -emit-ir2 -o - -pedantic-errors
// RUN: %not %gta3sc %s --config=gtasa --guesser -emit-ir2 -o - -pedantic-errors
{
LVAR_INT lvar

// CHECK-L: ADD_VAL_TO_INT_LVAR 0@ 1i8
// CHECK-L: IS_INT_LVAR_GREATER_OR_EQUAL_TO_NUMBER 0@ 5i8

REPEAT 5 lvar
    NOP
ENDREPEAT

TERMINATE_THIS_SCRIPT
}