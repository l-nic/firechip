# See LICENSE for license details.

#*****************************************************************************
# add.S
#-----------------------------------------------------------------------------
#
# Test add instruction.
#

#include "riscv_test.h"

# Register reads once the fifo register is added in
# will actually yield different data, but they'll
# just all see the same thing for now.
# Ask about the function that is applied on an
# individual neuron (it's not just multiply-add)
# Get the basic working first, and then figure out
# how it should be tested/changed.
# This is also a bit of an abuse of registers.
# We need to look at how to manage both registers
# and memory.
# This is also all 32-bit instructions

RVTEST_RV64U
RVTEST_CODE_BEGIN
  li x1, 0 # X1 wil be counter, start at 0
  li x2, 8 # X2 will be num_edges. Yes, this is a waste of a register.
  li x3, 0 # X3 will be accum
  li x5, 192 # This will be our "fifo head register"
  # We'll also reserve X6 to be the tail register
  la x10, testdata # The address of the weights
  nicread x12, x13, x14
  nicwrite x15, x16, x17
  jal zero, condition
loop:
  mv x8, x5 # Read the fifo head into x8
  lw x9, 0(x10) # Load the current weight
  # TODO: This is making all sorts of assumptions about the order in which bytes arrive
  # TODO: We should look into floats, as well
  mul x8, x8, x9
  add x3, x3, x8
  addi x10, x10, 4 #Go to the next weight in the array

  addi x1, x1, 1
condition:
  blt x1, x2, loop
  mv x6, x3

  li x4, 5376 # X4 will hold the correct result
  bne x6, x4, fail

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
