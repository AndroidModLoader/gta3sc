// RUN: %gta3sc %s --config=gta3 -emit-ir2 -o - | %FileCheck %s

DUMP
   // CHECK-NEXT-L: WAIT 127i8
   0100 04 7F
   // CHECK-NEXT-L: WAIT -1i8
   01 00 04 FF
   // CHECK-NEXT-L: WAIT -110i8
   01 00 04 92
   // CHECK-NEXT-L: SHAKE_CAM 0x0.000000p+0f
   0300 060000
ENDDUMP

// CHECK-NEXT-L: TERMINATE_THIS_SCRIPT
TERMINATE_THIS_SCRIPT
