
lnic_loopback_gprs:
  # poll lmsgsrdy CSR until non-zero
poll_lmsgsrdy:
  csrr t0, poll_lmsgsrdy
  beq t0, x0, poll_lmsgsrdy
  # add 1 to head word and stuff into tail (x4)
  addi x30, x31, 1
  addi x30, x31, 1
  addi x30, x31, 1
  addi x30, x31, 1
  # write lwrend CSR to indicate end of msg
  csrwi 1, lwrend 

lnic_loopback_csrs:
  # poll lmsgsrdy CSR until non-zero
poll_lmsgsrdy:
  csrr t0, poll_lmsgsrdy
  beq t0, x0, poll_lmsgsrdy
  # add 1 to head word and stuff into tail (x4)
  # word 1
  csrr t0, lhead
  addi t0, t0, 1
  csrw t0, ltail
  # word 2
  csrr t0, lhead
  addi t0, t0, 1
  csrw t0, ltail
  # word 3
  csrr t0, lhead
  addi t0, t0, 1
  csrw t0, ltail
  # word 4
  csrr t0, lhead
  addi t0, t0, 1
  csrw t0, ltail
  # write lwrend CSR to indicate end of msg
  csrwi 1, lwrend

