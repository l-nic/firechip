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
  csrrw zero, lread, t0
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
