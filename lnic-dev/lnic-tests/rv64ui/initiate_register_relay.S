# See LICENSE for license details.

#*****************************************************************************
# add.S
#-----------------------------------------------------------------------------
#
# Test add instruction.
#

#include "riscv_test.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN
  li x30, 0 # Write a zero to NIC out
loop:
  addi x30, x31, 1 # Relay back an incremented input
  jal zero, loop

  RVTEST_PASS
fail:
  RVTEST_FAIL
RVTEST_CODE_END

.data
testdata:
  .word 0
  .word 1
  .word 2
  .word 3
  .word 4
  .word 5
  .word 6
  .word 7

result:
  .dword -1

RVTEST_DATA_BEGIN
RVTEST_DATA_END
