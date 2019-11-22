cpu-loopback:     file format elf64-littleriscv

00000000000101e8 <reg_read16>:
   101e8:	fd010113          	addi	sp,sp,-48
   101ec:	02813423          	sd	s0,40(sp)
   101f0:	03010413          	addi	s0,sp,48
   101f4:	fca43c23          	sd	a0,-40(s0)
   101f8:	fd843783          	ld	a5,-40(s0)
   101fc:	fef43423          	sd	a5,-24(s0)
   10200:	fe843783          	ld	a5,-24(s0)
   10204:	0007d783          	lhu	a5,0(a5) # 0 <_start-0x100b0>
   10208:	03079793          	slli	a5,a5,0x30
   1020c:	0307d793          	srli	a5,a5,0x30
   10210:	00078513          	mv	a0,a5
   10214:	02813403          	ld	s0,40(sp)
   10218:	03010113          	addi	sp,sp,48
   1021c:	00008067          	ret

0000000000010220 <reg_read32>:
   10220:	fd010113          	addi	sp,sp,-48
   10224:	02813423          	sd	s0,40(sp)
   10228:	03010413          	addi	s0,sp,48
   1022c:	fca43c23          	sd	a0,-40(s0)
   10230:	fd843783          	ld	a5,-40(s0)
   10234:	fef43423          	sd	a5,-24(s0)
   10238:	fe843783          	ld	a5,-24(s0)
   1023c:	0007a783          	lw	a5,0(a5)
   10240:	0007879b          	sext.w	a5,a5
   10244:	00078513          	mv	a0,a5
   10248:	02813403          	ld	s0,40(sp)
   1024c:	03010113          	addi	sp,sp,48
   10250:	00008067          	ret

0000000000010254 <reg_write64>:
   10254:	fd010113          	addi	sp,sp,-48
   10258:	02813423          	sd	s0,40(sp)
   1025c:	03010413          	addi	s0,sp,48
   10260:	fca43c23          	sd	a0,-40(s0)
   10264:	fcb43823          	sd	a1,-48(s0)
   10268:	fd843783          	ld	a5,-40(s0)
   1026c:	fef43423          	sd	a5,-24(s0)
   10270:	fe843783          	ld	a5,-24(s0)
   10274:	fd043703          	ld	a4,-48(s0)
   10278:	00e7b023          	sd	a4,0(a5)
   1027c:	00000013          	nop
   10280:	02813403          	ld	s0,40(sp)
   10284:	03010113          	addi	sp,sp,48
   10288:	00008067          	ret

000000000001028c <nic_send_req_avail>:
   1028c:	ff010113          	addi	sp,sp,-16
   10290:	00113423          	sd	ra,8(sp)
   10294:	00813023          	sd	s0,0(sp)
   10298:	01010413          	addi	s0,sp,16
   1029c:	100167b7          	lui	a5,0x10016
   102a0:	01478513          	addi	a0,a5,20 # 10016014 <__global_pointer$+0x100040b4>
   102a4:	f7dff0ef          	jal	ra,10220 <reg_read32>
   102a8:	00050793          	mv	a5,a0
   102ac:	0007879b          	sext.w	a5,a5
   102b0:	0007879b          	sext.w	a5,a5
   102b4:	0ff7f793          	andi	a5,a5,255
   102b8:	0007879b          	sext.w	a5,a5
   102bc:	00078513          	mv	a0,a5
   102c0:	00813083          	ld	ra,8(sp)
   102c4:	00013403          	ld	s0,0(sp)
   102c8:	01010113          	addi	sp,sp,16
   102cc:	00008067          	ret

00000000000102d0 <nic_recv_req_avail>:
   102d0:	ff010113          	addi	sp,sp,-16
   102d4:	00113423          	sd	ra,8(sp)
   102d8:	00813023          	sd	s0,0(sp)
   102dc:	01010413          	addi	s0,sp,16
   102e0:	100167b7          	lui	a5,0x10016
   102e4:	01478513          	addi	a0,a5,20 # 10016014 <__global_pointer$+0x100040b4>
   102e8:	f39ff0ef          	jal	ra,10220 <reg_read32>
   102ec:	00050793          	mv	a5,a0
   102f0:	0007879b          	sext.w	a5,a5
   102f4:	0087d79b          	srliw	a5,a5,0x8
   102f8:	0007879b          	sext.w	a5,a5
   102fc:	0007879b          	sext.w	a5,a5
   10300:	0ff7f793          	andi	a5,a5,255
   10304:	0007879b          	sext.w	a5,a5
   10308:	00078513          	mv	a0,a5
   1030c:	00813083          	ld	ra,8(sp)
   10310:	00013403          	ld	s0,0(sp)
   10314:	01010113          	addi	sp,sp,16
   10318:	00008067          	ret

000000000001031c <nic_send_comp_avail>:
   1031c:	ff010113          	addi	sp,sp,-16
   10320:	00113423          	sd	ra,8(sp)
   10324:	00813023          	sd	s0,0(sp)
   10328:	01010413          	addi	s0,sp,16
   1032c:	100167b7          	lui	a5,0x10016
   10330:	01478513          	addi	a0,a5,20 # 10016014 <__global_pointer$+0x100040b4>
   10334:	eedff0ef          	jal	ra,10220 <reg_read32>
   10338:	00050793          	mv	a5,a0
   1033c:	0007879b          	sext.w	a5,a5
   10340:	0107d79b          	srliw	a5,a5,0x10
   10344:	0007879b          	sext.w	a5,a5
   10348:	0007879b          	sext.w	a5,a5
   1034c:	0ff7f793          	andi	a5,a5,255
   10350:	0007879b          	sext.w	a5,a5
   10354:	00078513          	mv	a0,a5
   10358:	00813083          	ld	ra,8(sp)
   1035c:	00013403          	ld	s0,0(sp)
   10360:	01010113          	addi	sp,sp,16
   10364:	00008067          	ret

0000000000010368 <nic_recv_comp_avail>:
   10368:	ff010113          	addi	sp,sp,-16
   1036c:	00113423          	sd	ra,8(sp)
   10370:	00813023          	sd	s0,0(sp)
   10374:	01010413          	addi	s0,sp,16
   10378:	100167b7          	lui	a5,0x10016
   1037c:	01478513          	addi	a0,a5,20 # 10016014 <__global_pointer$+0x100040b4>
   10380:	ea1ff0ef          	jal	ra,10220 <reg_read32>
   10384:	00050793          	mv	a5,a0
   10388:	0007879b          	sext.w	a5,a5
   1038c:	0187d79b          	srliw	a5,a5,0x18
   10390:	0007879b          	sext.w	a5,a5
   10394:	0007879b          	sext.w	a5,a5
   10398:	0ff7f793          	andi	a5,a5,255
   1039c:	0007879b          	sext.w	a5,a5
   103a0:	00078513          	mv	a0,a5
   103a4:	00813083          	ld	ra,8(sp)
   103a8:	00013403          	ld	s0,0(sp)
   103ac:	01010113          	addi	sp,sp,16
   103b0:	00008067          	ret

00000000000103b4 <nic_send>:
   103b4:	fd010113          	addi	sp,sp,-48
   103b8:	02113423          	sd	ra,40(sp)
   103bc:	02813023          	sd	s0,32(sp)
   103c0:	03010413          	addi	s0,sp,48
   103c4:	fca43c23          	sd	a0,-40(s0)
   103c8:	fcb43823          	sd	a1,-48(s0)
   103cc:	fd843703          	ld	a4,-40(s0)
   103d0:	fff00793          	li	a5,-1
   103d4:	0107d793          	srli	a5,a5,0x10
   103d8:	00f777b3          	and	a5,a4,a5
   103dc:	fef43423          	sd	a5,-24(s0)
   103e0:	fd043783          	ld	a5,-48(s0)
   103e4:	03079793          	slli	a5,a5,0x30
   103e8:	fe843703          	ld	a4,-24(s0)
   103ec:	00f767b3          	or	a5,a4,a5
   103f0:	fef43023          	sd	a5,-32(s0)
   103f4:	00000013          	nop
   103f8:	e95ff0ef          	jal	ra,1028c <nic_send_req_avail>
   103fc:	00050793          	mv	a5,a0
   10400:	fe078ce3          	beqz	a5,103f8 <nic_send+0x44>
   10404:	fe043583          	ld	a1,-32(s0)
   10408:	10016537          	lui	a0,0x10016
   1040c:	e49ff0ef          	jal	ra,10254 <reg_write64>
   10410:	00000013          	nop
   10414:	f09ff0ef          	jal	ra,1031c <nic_send_comp_avail>
   10418:	00050793          	mv	a5,a0
   1041c:	fe078ce3          	beqz	a5,10414 <nic_send+0x60>
   10420:	100167b7          	lui	a5,0x10016
   10424:	01078513          	addi	a0,a5,16 # 10016010 <__global_pointer$+0x100040b0>
   10428:	dc1ff0ef          	jal	ra,101e8 <reg_read16>
   1042c:	00000013          	nop
   10430:	02813083          	ld	ra,40(sp)
   10434:	02013403          	ld	s0,32(sp)
   10438:	03010113          	addi	sp,sp,48
   1043c:	00008067          	ret

0000000000010440 <nic_recv>:
   10440:	fd010113          	addi	sp,sp,-48
   10444:	02113423          	sd	ra,40(sp)
   10448:	02813023          	sd	s0,32(sp)
   1044c:	03010413          	addi	s0,sp,48
   10450:	fca43c23          	sd	a0,-40(s0)
   10454:	fd843783          	ld	a5,-40(s0)
   10458:	fef43423          	sd	a5,-24(s0)
   1045c:	00000013          	nop
   10460:	e71ff0ef          	jal	ra,102d0 <nic_recv_req_avail>
   10464:	00050793          	mv	a5,a0
   10468:	fe078ce3          	beqz	a5,10460 <nic_recv+0x20>
   1046c:	fe843583          	ld	a1,-24(s0)
   10470:	100167b7          	lui	a5,0x10016
   10474:	00878513          	addi	a0,a5,8 # 10016008 <__global_pointer$+0x100040a8>
   10478:	dddff0ef          	jal	ra,10254 <reg_write64>
   1047c:	00000013          	nop
   10480:	ee9ff0ef          	jal	ra,10368 <nic_recv_comp_avail>
   10484:	00050793          	mv	a5,a0
   10488:	fe078ce3          	beqz	a5,10480 <nic_recv+0x40>
   1048c:	100167b7          	lui	a5,0x10016
   10490:	01278513          	addi	a0,a5,18 # 10016012 <__global_pointer$+0x100040b2>
   10494:	d55ff0ef          	jal	ra,101e8 <reg_read16>
   10498:	00050793          	mv	a5,a0
   1049c:	fef42223          	sw	a5,-28(s0)
   104a0:	0ff0000f          	fence
   104a4:	fe442783          	lw	a5,-28(s0)
   104a8:	00078513          	mv	a0,a5
   104ac:	02813083          	ld	ra,40(sp)
   104b0:	02013403          	ld	s0,32(sp)
   104b4:	03010113          	addi	sp,sp,48
   104b8:	00008067          	ret

00000000000104bc <main>:
   104bc:	fd010113          	addi	sp,sp,-48
   104c0:	02113423          	sd	ra,40(sp)
   104c4:	02813023          	sd	s0,32(sp)
   104c8:	03010413          	addi	s0,sp,48
   104cc:	fd840793          	addi	a5,s0,-40
   104d0:	00078513          	mv	a0,a5
   104d4:	f6dff0ef          	jal	ra,10440 <nic_recv>
   104d8:	fe042623          	sw	zero,-20(s0)
   104dc:	0400006f          	j	1051c <main+0x60>
   104e0:	fec42783          	lw	a5,-20(s0)
   104e4:	00279793          	slli	a5,a5,0x2
   104e8:	ff040713          	addi	a4,s0,-16
   104ec:	00f707b3          	add	a5,a4,a5
   104f0:	fe87a783          	lw	a5,-24(a5)
   104f4:	0017879b          	addiw	a5,a5,1
   104f8:	0007871b          	sext.w	a4,a5
   104fc:	fec42783          	lw	a5,-20(s0)
   10500:	00279793          	slli	a5,a5,0x2
   10504:	ff040693          	addi	a3,s0,-16
   10508:	00f687b3          	add	a5,a3,a5
   1050c:	fee7a423          	sw	a4,-24(a5)
   10510:	fec42783          	lw	a5,-20(s0)
   10514:	0017879b          	addiw	a5,a5,1
   10518:	fef42623          	sw	a5,-20(s0)
   1051c:	fec42783          	lw	a5,-20(s0)
   10520:	0007871b          	sext.w	a4,a5
   10524:	00300793          	li	a5,3
   10528:	fae7dce3          	ble	a4,a5,104e0 <main+0x24>
   1052c:	fd840793          	addi	a5,s0,-40
   10530:	01000593          	li	a1,16
   10534:	00078513          	mv	a0,a5
   10538:	e7dff0ef          	jal	ra,103b4 <nic_send>
   1053c:	00000793          	li	a5,0
   10540:	00078513          	mv	a0,a5
   10544:	02813083          	ld	ra,40(sp)
   10548:	02013403          	ld	s0,32(sp)
   1054c:	03010113          	addi	sp,sp,48
   10550:	00008067          	ret
