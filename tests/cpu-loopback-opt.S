
#define PKT_LEN 256

main:
  # init pkt_buf on stack
  mv s0, sp # t0 = &pkt_buf
  sd s0, PKT_LEN(sp)
  addi sp, sp, -(PKT_LEN + 8)
  # poll nic_recv_req_avail() address until non-zero
poll_nic_recv_req_avail:
  ld t0, NIC_RECV_REQ_AVAIL
  beq t0, x0, poll_nic_recv_req_avail
  # write SIMPLENIC_RECV_REQ address with &pkt_buf
  sd s0, SIMPLENIC_RECV_REQ
  # poll nic_recv_comp_avail() address until non-zero
poll_nic_recv_comp_avail:
  ld t0, NIC_RECV_COMP_AVAIL
  beq t0, x0, poll_nic_recv_comp_avail
  # read SIMPLENIC_RECV_COMP address to get pkt_len
  ld t0, SIMPLENIC_RECV_COMP
  # fence
  fence
  # increment word 1
  ld t0, 0(s0)
  addi t0, t0, 1
  sd t0, 0(s0)
  # increment word 2
  ld t0, 8(s0)
  addi t0, t0, 1
  sd t0, 8(s0)
  # increment word 3
  ld t0, 16(s0)
  addi t0, t0, 1
  sd t0, 16(s0)
  # increment word 4
  ld t0, 24(s0)
  addi t0, t0, 1
  sd t0, 24(s0)
  # construct send descriptor (pkt_len (16-bit) ++ &pkt_buf (48-bit))
  li t1, PKT_LEN
  slli t1, t1, 48
  or t1, t1, s0
  # poll nic_send_req_avail() address until non-zero
poll_nic_send_req_avail:
  ld t0, NIC_SEND_REQ_AVAIL
  beq t0, x0, poll_nic_send_req_avail
  # write SIMPLENIC_SEND_REQ address with send descriptor
  sd t1, SIMPLENIC_SEND_REQ
  # poll nic_send_comp_avail() address until non-zero
poll_nic_send_comp_avail:
  ld t0, NIC_SEND_COMP_AVAIL
  beq t0, x0, poll_nic_send_comp_avail
  # read SIMPLENIC_SEND_COMP address
  ld t0, SIMPLENIC_SEND_COMP
  # restore stack variables and free pkt_buf
  ld s0, PKT_LEN(sp)
  addi sp, sp, (PKT_LEN + 8)
