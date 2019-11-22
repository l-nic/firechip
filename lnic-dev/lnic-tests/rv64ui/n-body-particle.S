# See LICENSE for license details.

#*****************************************************************************
# n-body-particle.S
#-----------------------------------------------------------------------------
#
# Prototype of a particle node in the N-body nanoservice application in RISC-V assembly.
#

#include "riscv_test.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

# MsgHdr format:
#   msg_type - type of msg (StartTraversal, TraversalReq, TraversalResp, StartUpdate, Update, Convergence)
#   src
#   dst

# TraversalReq Format:
#   pos
#   traversal_src
#   num_expected
#   expected
#   num_sources
#   sources

# TraversalResp Format:
#   force
#   num_expected
#   expected
#   num_sources
#   sources

# Update Format:
#   update_src
#   pos

#############################
# Particle Node Pseudo Code #
#############################
# NOTE: ignore node replication for now

# Receive msg from network:
#   If StartTraversal:
#     Send TraversalReq to root node
#   Elif TraversalReq:
#     Process request (i.e. compute force) and send back TraversalResp
#   Elif TraversalResp:
#     Accumulate force indicated in the response
#     Update response_bitmap
#     If converged: mark as converged
#   Elif StartUpdate:
#     Check that we've converged
#     Reset response_bitmap
#     Update particle position using net force
#     Send Update msg to parent
#   Elif Convergence:
#     Send StartUpdate msg to neighbor (if we have one)

# Particle state:
#   particle_id
#   pos
#   neighbor_id
#   parent
#   root
#   is_converged
#   total_force
#   response_bitmap (hash map)

particle_node_start:
  # Initialize state
  # allocate 7 words (7*8 byte) struct on heap (result pointer stored in x10)
  li x10, 7*8
  jal x1, malloc
  mv x8, x10 # save particle_state ptr
  # store particle_id, pos, neighbor_id, parent_id, root_id, is_converged, total_force in memory
  sd x11, 0(x8)
  sd x12, 8(x8)
  sd x13, 2*8(x8)
  sd x14, 3*8(x8)
  sd x15, 4*8(x8)
  sd x0, 5*8(x8)
  sd x0, 6*8(x8)
  # allocate hash_map on heap (result pointer stored in x10)
  li x10, HEAP_BYTES
  jal x1, malloc
  mv x9, x10 # save response_bitmap ptr
start_rcv:
  # wait for msg and store the msg_type
  mv x5, x31
  li x6, TRAVERSAL_REQ_TYPE
  beq x5, x6, process_traversal_req
  li x6, TRAVERSAL_RESP_TYPE
  beq x5, x6, process_traversal_resp
  li x6, START_UPDATE_TYPE
  beq x5, x6, start_update
  li x6, CONVERGENCE_TYPE
  beq x5, x6, process_convergence

start_traversal:
  # discard src and dst from msg
  mv x0, x31
  mv x0, x31
  # send TraversalReq to root node
  # write msg_type
  li x30, TRAVERSAL_REQ_TYPE
  # write src (particle_id)
  ld x30, 0(x8)
  # write dst (root_id)
  ld x30, 4*64(x8)
  # write pos
  ld x30, 64(x8)
  # write traversal_src (particle_id)
  ld x30, 0(x8)
  # write num_expected
  li x30, 1
  # write expected list (root node)
  ld x30, 4*64(x8)
  # write num_sources
  li x30, 0
  li x30, MSG_END
  # wait for next msg
  jal x0, start_rcv

process_traversal_req:
  # discard src and dst from msg
  mv x0, x31
  mv x0, x31
  # compute force on particle (result stored in x10)
  mv x10, x31 # move pos into argument reg
  jal x1, compute_force
  # send back TraversalResp
  # write msg_type
  li x30, TRAVERSAL_RESP_TYPE
  # write src (particle_id)
  ld x30, 0(x8)
  # write dst (msg.traversal_src)
  mv x30, x31
  # write force into the response
  mv x30, x10
  mv x5, x31 # store msg.num_expected
  # write msg.num_expected into the response
  mv x30, x5
write_expected:
  mv x30, x31
  addi x5, x5, -1
  # loop to write all msg.expected
  bne x0, x5, write_expected
  mv x5, x31 # store num_sources
  # write num_sources msg.num_sources + 1
  addi x30, x5, 1
  # write sources (particle_id ++ msg.sources)
  ld x30, 0(x8) # write particle_id
write_src:
  mv x30, x31
  addi x5, x5, -1
  # loop to write all msg.sources
  bne x0, x5, write_src
  # done writing response
  li x30, MSG_END
  # wait for next msg
  jal x0, start_rcv

process_traversal_resp:
  # discard msg.src and msg.dst
  mv x0, x31
  mv x0, x31
  # accumulate force
  ld x5, 6*8(x8) # x5 = total_force
  add x5, x5, x31 # x5 = x5 + msg.force
  sd x5, 6*8(x8) # save result back into memory
  # update response_bitmap
  mv x18, x31 # store msg.num_expected in saved register
  # iterate through all msg.expected values and set to 0 if nodeID not in the hash_map
update_response_bitmap:
  mv x10, x9 # store response_bitmap ptr in argument reg
  mv x11, x31 # store nodeID in argument reg
  jal x1, check_hash_map # check if nodeID in hash_map (result stored in x10)  
  bne x0, x10, skip
  # set response_bitmap[nodeID] = 0
  li x12, 0
  jal x1, set_hash_map
  addi x18, x18, -1 # decrement counter
skip:
  bne x0, x18, update_response_bitmap
  mv x18, x31 # store msg.num_sources in saved register
  # iterate through msg.sources and set response_bitmap[nodeID] = 1
set_response_bitmap:
  mv x11, x31 # store nodeID in argument reg
  li x12, 1 # put 1 in argument reg
  jal x1, set_hash_map
  addi x18, x18, -1
  bne x0, x18, set_response_bitmap
  # check if len(response_bitmap) == sum(response_bitmap.values())
  jal x1, check_converged # result stored in x10
  beq x0, x10, start_rcv # if not converged, process next msg
  sd x10, 5*8(x8) # record that the node has converged 
  jal x1, start_rcv

start_update:
  # discard msg src and dst
  mv x0, x31
  mv x0, x31
  # check that we've converged
  ld x5, 5*8(x8)
  beq x0, x5, start_rcv # move onto next msg if we haven't converged yet
  # reset response_bitmap
  mv x10, x9
  jal x1, reset_hash_map
  # reset is_converged
  sd x0, 5*8(x8)
  ld x10, 6*8(x8) # load net force into argument reg
  ld x11, 8(x8) # load pos into argument reg
  jal x1, update_position # new pos stored in x10
  sd x10, 8(x8) # save new position
  # send update msg to parent
  li x30, UPDATE_TYPE
  ld x30, 0(x8) # write src (particle_id)
  ld x30, 3*8(x8) # write dst (parent_id)
  ld x30, 0(x8) # write update_src (particle_id)
  ld x30, 8(x8) # write pos
  li x30, MSG_END
  jal x0, start_rcv # process next msg

process_convergence:
#   Send StartUpdate msg to neighbor (if we have one)
  # discard msg src and dst
  mv x0, x31
  mv x0, x31
  li x5, NULL_ID
  ld x6, 2*8(x8) # x6 = neighbor_id
  # if we don't have a neighbor don't do anything
  beq x5, x6, start_rcv
  # send StartUpdate msg to neighbor
  li x30, START_UPDATE_TYPE # write msg_type
  ld x30, 0(x8) # write src (particle_id)
  mv x30, x6 # write dst (neighbor_id)
  li x30, MSG_END
  jal, x0, start_rcv

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
