// RUN: %gta3sc %s --config=gtasa --guesser --cm -emit-ir2 -o - | %FileCheck %s

MISSION_START
// CHECK-NEXT-L: GOSUB %MAIN_1
GOSUB test_offset
// CHECK-NEXT-L: TERMINATE_THIS_SCRIPT
MISSION_END

{
// CHECK-NEXT-L: MAIN_1:
test_offset:

LVAR_INT i j k

// CHECK-NEXT-L: SET_LVAR_INT 32@ 0i8
timera = 0

// CHECK-NEXT-L: SET_LVAR_INT 34@ 100i8
i = 100

// CHECK-NEXT-L: TERMINATE_THIS_SCRIPT
TERMINATE_THIS_SCRIPT // testing if terminate is allowed
}
