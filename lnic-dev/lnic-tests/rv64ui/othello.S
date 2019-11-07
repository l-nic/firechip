# See LICENSE for license details.

#*****************************************************************************
# othello.S
#-----------------------------------------------------------------------------
#
# Prototype of the Othello nanoservice application in RISC-V assembly.
#

#include "riscv_test.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

# OthelloHdr format:
#   msg_type (8 bits) - type of msg (either MapMsg or ReduceMsg)

# MapMsg Format:
#   board - othello game board
#   max_depth - max depth into the game tree this map message should propagate
#   cur_depth - how deep into the game tree this msg currently is
#   src_host_id - ID of the host that generated this msg
#   src_msg_ptr - ptr to the MsgState on the src_host

# ReduceMsg Format:
#   target_host_id - ID of the host that this msg should be sent to
#   target_msg_ptr - ptr to the MsgState on the target_host
#   minimax_val - the min (or max) value sent up the tree

# Othello MsgState:
#   src_host_id - who to send the result to once all responses arrive
#   src_msg_ptr - ptr to the MsgState on the src_host
#   map_cnt - number of messages (i.e. boards) that were generated in response to processing msg_id
#   response_cnt - number of the responses that have arrived so far (incremented during the reduce phase
#   minimax_val - the running min/max value of the responses

###############################
# Othello Program Pseudo Code #
###############################
# Receive and parse msg from the network
# If MapMsg:
#   Compute new boards
#   If the desired depth has been reached:
#     Evalute the boards and compute running min (or max)
#     Send ReduceMsg with min (or max) value to parent
#   Else: # depth has not been reached yet
#     Record MsgState so that we know when we've received all the responses
#     Send out new MapMsgs into the network
# Else: # it is a ReduceMsg
#   Lookup MsgState associated with the msg
#   If all responses have been received:
#     Send ReduceMsg with min (or max) value up to parent

othello:
  # branch if FIFO head == 0 (i.e. is a reduce msg)
  beq x0, x31, process_reduce_msg
process_map_msg:
  # move FIFO head (the board) into the argument reg
  mv x10, x31
  # call compute_boards procedure (pointer to resulting boards stored in x10, number of boards in x11)
  jal x1, compute_boards
  # read max_depth from FIFO head
  mv x18, x31
  # read cur_depth from the FIFO head
  mv x19, x31
  # branch if cur_depth < max_depth
  blt x19, x18, send_boards
  # cur_depth = max_depth
  # call evaluate_boards to compute the min (or max) value (result stored in x10)
  jal x1, evaluate_boards
  # construct reduce msg and send into network
  # write target_host_id, target_msg_ptr into network
  mv x30, x31
  mv x30, x31
  # append minimax value
  mv x30, x10
  li x30, MSG_END
send_boards:
  # record msg state
  # save boards and num_boards
  mv x8, x10
  mv x9, x11
  # allocate 5 words (5*8 bytes) on the heap
  li x10, 5*8
  jal x1, malloc # resulting pointer stored in x10
  # store src_host_id
  sd x31, 0(x10)
  # store src_msg_ptr
  sd x31, 64(x10)
  # store map_cnt (i.e. num_boards)
  sd x9, 2*64(x10)
  # store response_cnt
  sd x0, 3*64(x10)
  # store initial minimax_val
  sd MAX_INT, 4*64(x10)
  # send new MapMsgs into the network (one per board)
  mv x5, x9 # x5 = counter from num_boards down to 0
send_map_msg:
  # decrement counter
  subi x5, x5, 1
  ld x30, x5(x8) # append board to Tail FIFO
  mv x30, x18 # append max_depth
  addi x30, x19, 1 # append cur_depth + 1
  li x30, HOST_ID # append hostID
  mv x30, x10 # append msg_ptr
  li x30, MSG_END
  # loop if needed
  bne x0, x5, send_map_msg 
process_reduce_msg:
  # discard target_host_id (used for routing, perhaps removed by NIC)
  mv x0, x31
  # x5 = MsgState ptr from the msg
  mv x5, x31
  # lookup map_cnt
  ld x6, 2*64(x5)
  # lookup response_cnt
  ld x7, 3*64(x5)
  # lookup minimax_val
  ld x28, 4*64(x5)
  mv x29, x31 # x29 = msg minimax_val
  # compute running minimum
  bge x29, x28, skip
  sd x29, 4*64(x5)
skip:
  # check if all responses have been received
  bne x6, x7, pass
  # send ReduceMsg to parent
  # lookup src_host_id and write to network
  ld x30, 0(x5)
  # lookup src_msg_ptr and write to network
  ld x30, 64(x5)
  # lookup minimax_val and write to network
  ld x30, 4*64(x5)
  # free MsgState (5*8 bytes)
  li x10, 5*8
  jal x1, free
  jal x0, pass

compute_boards:
  # TODO: given an initial board, compute new boards
  # store pointer to boards in x10 and num_boards in x11

pass:
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
