// RUN: %gta3sc %s --config=gta3 -moptimize-andor -emit-ir2 -o - | %FileCheck %s

VAR_INT x

// CHECK-NOT-L: ANDOR 0i8
// CHECK-L:     IS_INT_VAR_EQUAL_TO_NUMBER &8 0i8
IF x = 0
    WAIT 0
ENDIF

// CHECK-NOT-L: ANDOR 0i8
// CHECK-L:     IS_INT_VAR_EQUAL_TO_NUMBER &8 0i8
WHILE x = 0
    WAIT 0
ENDWHILE

TERMINATE_THIS_SCRIPT

