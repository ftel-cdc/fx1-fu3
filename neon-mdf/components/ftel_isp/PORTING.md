# FX1 ISP import provenance

Snapshot imported 2026-11-04 from the workspace FX1_ISP and fx1-dev V3.0 delivery. No external source path is used at configure, build or runtime. API and source contents are preserved. FX1 memory map is VP provisional, pending HAS. Startup and linker target DDR preinit; no bootloader or VP runner is built.

| Original | Local | SHA-256 at import |
|---|---|---|
| `fx1-dev/sw/bootloader/sources/VP_FX1_V3.0/bsp/crt0.S` | `neon-mdf/arch/RISCV/Device/FTEL/FX1/Source/gcc/startup_FX1.S` | `433a9fa18910c9fce7345ddf2f15b07c5746cd17f125766a242bca44ac66a9a2` |
| `fx1-dev/sw/bootloader/sources/VP_FX1_V3.0/bsp/fx1.ld.S` | `neon-mdf/arch/RISCV/Device/FTEL/FX1/Source/gcc/fx1.ld.S` | `b5ced0e894739576956db3bd873bef12014228f070cf8d955403d72ded81e53c` |
| `fx1-dev/sw/bootloader/sources/VP_FX1_V3.0/bsp/fx1_fw.c` | `neon-mdf/arch/RISCV/Device/FTEL/FX1/Source/fx1_fw.c` | `9f76a0301a79a19b16e4450208075fa258b1da3b74b12c5b5e7ff341dbcdf9af` |
| `fx1-dev/sw/bootloader/sources/VP_FX1_V3.0/bsp/fx1_fw.h` | `neon-mdf/arch/RISCV/Device/FTEL/FX1/Include/fx1_fw.h` | `2021f717a15a51434f18edb7f3dad610558c34b6839417be74932e95466b37eb` |
| `fx1-dev/sw/bootloader/sources/VP_FX1_V3.0/include/fx1/fx1_memory_map.h` | `neon-mdf/arch/RISCV/Device/FTEL/FX1/Include/fx1/fx1_memory_map.h` | `9656c8bd79bc9baf4cf3f9b0fd1c30584c2718dcfae6125dd1af82dacf7c8003` |
| `fx1-dev/sw/bootloader/sources/VP_FX1_V3.0/app/rtos_libc.c` | `neon-mdf/arch/RISCV/Device/FTEL/FX1/Source/fx1_libc.c` | `b3c65e1ef4908b25aca3a6ecf6cbc2a226de12c28d29b7146446aa3051348708` |
| `FX1_ISP/common/ftel_isp_common.h` | `neon-mdf/components/ftel_isp/common/ftel_isp_common.h` | `53503bd89920e577d1925ec24d387114284d5c6ac351de5f2b119616ad675b65` |
| `FX1_ISP/common/ftel_isp_common.c` | `neon-mdf/components/ftel_isp/common/ftel_isp_common.c` | `8e8c4395b4b51c0809b0cc0b00eebe65a2f1505a2148b5ebdf70a7dc52bd3ce7` |
| `FX1_ISP/pipeline/ftel_isp_init.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_init.h` | `24203595c1531d5b64cbbb9603958dc06d7ef109647fd344a15370e37aa26d8a` |
| `FX1_ISP/pipeline/ftel_isp_stream.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_stream.h` | `5fb04f8d21facfbf29b69c39f0b9ec533128a350bf2f220f5936f27a4c567e37` |
| `FX1_ISP/pipeline/ftel_isp_stream.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_stream.c` | `0775f3b155f37dd5683c9a00bd89dbbde5829553202252843e089d66bd110191` |
| `FX1_ISP/pipeline/ftel_isp_init.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_init.c` | `325f4b58e068b9f507dd6dd04c29654e724e8aeead42651b2a6cc7e8e4c193c6` |
| `FX1_ISP/pipeline/ftel_isp_3a.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_3a.c` | `0dc1225e900a6bf6546f3343318104de56092ba9bd2268aa750f4cf520e29622` |
| `FX1_ISP/pipeline/ftel_isp_3a.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_3a.h` | `2fe859e2c25f862d2aaa6d47ff0debc686b64179914ade8a5f2ce866288f478a` |
| `FX1_ISP/pipeline/ftel_isp_init.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_init.h` | `24203595c1531d5b64cbbb9603958dc06d7ef109647fd344a15370e37aa26d8a` |
| `FX1_ISP/pipeline/ftel_isp_stream.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_stream.h` | `5fb04f8d21facfbf29b69c39f0b9ec533128a350bf2f220f5936f27a4c567e37` |
| `FX1_ISP/pipeline/ftel_isp_stream.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_stream.c` | `0775f3b155f37dd5683c9a00bd89dbbde5829553202252843e089d66bd110191` |
| `FX1_ISP/pipeline/ftel_isp_init.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_init.c` | `325f4b58e068b9f507dd6dd04c29654e724e8aeead42651b2a6cc7e8e4c193c6` |
| `FX1_ISP/pipeline/ftel_isp_3a.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_3a.c` | `0dc1225e900a6bf6546f3343318104de56092ba9bd2268aa750f4cf520e29622` |
| `FX1_ISP/pipeline/ftel_isp_3a.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_3a.h` | `2fe859e2c25f862d2aaa6d47ff0debc686b64179914ade8a5f2ce866288f478a` |
| `FX1_ISP/pipeline/ftel_isp_init.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_init.h` | `24203595c1531d5b64cbbb9603958dc06d7ef109647fd344a15370e37aa26d8a` |
| `FX1_ISP/pipeline/ftel_isp_stream.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_stream.h` | `5fb04f8d21facfbf29b69c39f0b9ec533128a350bf2f220f5936f27a4c567e37` |
| `FX1_ISP/pipeline/ftel_isp_stream.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_stream.c` | `0775f3b155f37dd5683c9a00bd89dbbde5829553202252843e089d66bd110191` |
| `FX1_ISP/pipeline/ftel_isp_init.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_init.c` | `325f4b58e068b9f507dd6dd04c29654e724e8aeead42651b2a6cc7e8e4c193c6` |
| `FX1_ISP/pipeline/ftel_isp_3a.c` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_3a.c` | `0dc1225e900a6bf6546f3343318104de56092ba9bd2268aa750f4cf520e29622` |
| `FX1_ISP/pipeline/ftel_isp_3a.h` | `neon-mdf/components/ftel_isp/pipeline/ftel_isp_3a.h` | `2fe859e2c25f862d2aaa6d47ff0debc686b64179914ade8a5f2ce866288f478a` |
| `FX1_ISP/bayer domain/ifmt/ftel_isp_ifmt.c` | `neon-mdf/components/ftel_isp/bayer domain/ifmt/ftel_isp_ifmt.c` | `04451612ee43ed8151b703c4eb9c65e5107ed31c158cebebdeaf2ac34382def6` |
| `FX1_ISP/bayer domain/ifmt/ftel_isp_ifmt.h` | `neon-mdf/components/ftel_isp/bayer domain/ifmt/ftel_isp_ifmt.h` | `60e1141df8cf9f607211f08b852ecb156df3322b69c3b211d1f28898e74729f1` |
| `FX1_ISP/bayer domain/blc/ftel_isp_blc.c` | `neon-mdf/components/ftel_isp/bayer domain/blc/ftel_isp_blc.c` | `d36c7aae5366f71b5b576bab61e22e5c81d488cc925340d29f20cdb059484f49` |
| `FX1_ISP/bayer domain/blc/ftel_isp_blc.h` | `neon-mdf/components/ftel_isp/bayer domain/blc/ftel_isp_blc.h` | `b046b30f2ea27223753539a2e491aa9e0efef1c8f24026db9273dc551028a41a` |
| `FX1_ISP/bayer domain/lsc/ftel_isp_lsc.h` | `neon-mdf/components/ftel_isp/bayer domain/lsc/ftel_isp_lsc.h` | `2f222552c95cb2165836e21de758dcf7a3b78ae9283f2d1d23a7ddb149f320c0` |
| `FX1_ISP/bayer domain/lsc/ftel_isp_lsc.c` | `neon-mdf/components/ftel_isp/bayer domain/lsc/ftel_isp_lsc.c` | `ad193c754cbe7ebe1e81dba3929d137ad7d578ef92c486d3d37fbb0c3e72e28e` |
| `FX1_ISP/bayer domain/bpc/ftel_isp_bpc.h` | `neon-mdf/components/ftel_isp/bayer domain/bpc/ftel_isp_bpc.h` | `b0c9fa77aa3c37f1b44ec9cdc2bf6e564e5ddffd4848a0be33a86df5256847dd` |
| `FX1_ISP/bayer domain/bpc/ftel_isp_bpc.c` | `neon-mdf/components/ftel_isp/bayer domain/bpc/ftel_isp_bpc.c` | `806b7dc43987d6ef260726060ec33881b4480316932ac453be52628256ba54e8` |
| `FX1_ISP/bayer domain/wb/ftel_isp_wb.h` | `neon-mdf/components/ftel_isp/bayer domain/wb/ftel_isp_wb.h` | `fc1f6a32594648895ad3f99f063bf0795952d0a168f13a68c089bcd293232f12` |
| `FX1_ISP/bayer domain/wb/ftel_isp_wb.c` | `neon-mdf/components/ftel_isp/bayer domain/wb/ftel_isp_wb.c` | `e9abd575d94e9d5cc71bc0690777629f9d5b9c300f4c5f7a609ecd25296d1cb6` |
| `FX1_ISP/bayer domain/dg/ftel_isp_dg.h` | `neon-mdf/components/ftel_isp/bayer domain/dg/ftel_isp_dg.h` | `1f9cf2ecebce111c0281cf888ad2283c37b52007cca14f416b21c31fd2a148e3` |
| `FX1_ISP/bayer domain/dg/ftel_isp_dg.c` | `neon-mdf/components/ftel_isp/bayer domain/dg/ftel_isp_dg.c` | `ae5c6f9536cc832be1b993926c3f459f05aa1f524a2b8dcead34dc31053e474f` |
| `FX1_ISP/rgb domain/demosaic/ftel_isp_demosaic.h` | `neon-mdf/components/ftel_isp/rgb domain/demosaic/ftel_isp_demosaic.h` | `3b9f522104ebacd3e6a6aefc078c7b9b01d5e067e2ca4d645709923d0bc911fe` |
| `FX1_ISP/rgb domain/demosaic/ftel_isp_demosaic.c` | `neon-mdf/components/ftel_isp/rgb domain/demosaic/ftel_isp_demosaic.c` | `b942af3d8e97b22a61866ae1e22237b12a488efc82e2cbab6c75b32cfe94fdb7` |
| `FX1_ISP/rgb domain/ccm/ftel_isp_ccm.h` | `neon-mdf/components/ftel_isp/rgb domain/ccm/ftel_isp_ccm.h` | `bb0cb1754e3a47a19be2905a6dd6042feeb5006122b2ccfd6298ef1684e3ad68` |
| `FX1_ISP/rgb domain/ccm/ftel_isp_ccm.c` | `neon-mdf/components/ftel_isp/rgb domain/ccm/ftel_isp_ccm.c` | `924c0aecb09927da2d2b704cd82acbae2ad6a6c13a2ca362e66f3096fdd62416` |
| `FX1_ISP/rgb domain/gamma/ftel_isp_gamma.h` | `neon-mdf/components/ftel_isp/rgb domain/gamma/ftel_isp_gamma.h` | `aa1d82f0512459b58f6ca26dc2223e1b44b861c55e33f5bdb16edd8f9bc45d1f` |
| `FX1_ISP/rgb domain/gamma/ftel_isp_gamma.c` | `neon-mdf/components/ftel_isp/rgb domain/gamma/ftel_isp_gamma.c` | `53de12d9014f366b47739326ee506bfc52758107fcc49226ee4966769d8d64b0` |
| `FX1_ISP/rgb domain/csc/ftel_isp_csc.c` | `neon-mdf/components/ftel_isp/rgb domain/csc/ftel_isp_csc.c` | `e15704020595dfa2d14ae2592a625eccf2e16593cf642e48dc5c4ebd3f600660` |
| `FX1_ISP/rgb domain/csc/ftel_isp_csc.h` | `neon-mdf/components/ftel_isp/rgb domain/csc/ftel_isp_csc.h` | `828b8cc63ffd62afe4ce13c25bd1d738367fe92702b0a212fb3042fcdb426ecb` |
| `FX1_ISP/yuv domain/gtm/ftel_isp_gtm.c` | `neon-mdf/components/ftel_isp/yuv domain/gtm/ftel_isp_gtm.c` | `c2ab4b7eddfe0b8ee613b905993f6f2fa75e4ceca400ba904da1c550d412cd65` |
| `FX1_ISP/yuv domain/gtm/ftel_isp_gtm.h` | `neon-mdf/components/ftel_isp/yuv domain/gtm/ftel_isp_gtm.h` | `b4c4006bdf84e9ff972720b51150d015947ac20c18ea6238ef956181c85d6dde` |
| `FX1_ISP/yuv domain/nr/ftel_isp_nr.h` | `neon-mdf/components/ftel_isp/yuv domain/nr/ftel_isp_nr.h` | `b8d45210d1d35b9d73fb0a33de1dc4806a8f02248e8f44c174cc65247bad668b` |
| `FX1_ISP/yuv domain/nr/ftel_isp_nr.c` | `neon-mdf/components/ftel_isp/yuv domain/nr/ftel_isp_nr.c` | `c2141931bacd8b04ffb2de41c74325580d0efc8f4515ac306f0c8f968302384d` |
| `FX1_ISP/yuv domain/ee/ftel_isp_ee.c` | `neon-mdf/components/ftel_isp/yuv domain/ee/ftel_isp_ee.c` | `b1334b7db2bdfd9d6ba152907ab0331685228b1c88e7aac1b2eaee50dba4ad7e` |
| `FX1_ISP/yuv domain/ee/ftel_isp_ee.h` | `neon-mdf/components/ftel_isp/yuv domain/ee/ftel_isp_ee.h` | `f599a1e42f92d9b4358d73f3f1cd0db2105b1ae6e9d285cd1af344fb2639a3d9` |
| `FX1_ISP/yuv domain/cnf/ftel_isp_cnf.c` | `neon-mdf/components/ftel_isp/yuv domain/cnf/ftel_isp_cnf.c` | `c32d823c97d636864a4143c507957da5fefb85da248d05961d513483f1da3511` |
| `FX1_ISP/yuv domain/cnf/ftel_isp_cnf.h` | `neon-mdf/components/ftel_isp/yuv domain/cnf/ftel_isp_cnf.h` | `3b18da035a689a9ad0ccfa141a25dce3da979c259426dcb9d759d9953b0a12eb` |
| `FX1_ISP/stats/aec/ftel_isp_aec.c` | `neon-mdf/components/ftel_isp/stats/aec/ftel_isp_aec.c` | `72649eca3440eaf6a447285013f99e72439ed3279a61803b9a804d73a609e969` |
| `FX1_ISP/stats/aec/ftel_isp_aec.h` | `neon-mdf/components/ftel_isp/stats/aec/ftel_isp_aec.h` | `9e3176a8644af1df4f3807c9157393b5036b05f1fda650d840d9b79eea58eea6` |
| `FX1_ISP/stats/awb/ftel_isp_awb.h` | `neon-mdf/components/ftel_isp/stats/awb/ftel_isp_awb.h` | `4e71fc2a6b25fb309a57bd382a436025fb12504c99db8784bad6c412e6a8bebd` |
| `FX1_ISP/stats/awb/ftel_isp_awb.c` | `neon-mdf/components/ftel_isp/stats/awb/ftel_isp_awb.c` | `b5d7c616cc7b3e1adad4c598d25ec53a676a8bd98e172ec987f4a50fe3333f8a` |
| `FX1_ISP/stats/af/ftel_isp_af.h` | `neon-mdf/components/ftel_isp/stats/af/ftel_isp_af.h` | `a9f2b91de5b2bbf008e45bf6562d03fd09af779e39f011dd081f2bbdb5ea44c6` |
| `FX1_ISP/stats/af/ftel_isp_af.c` | `neon-mdf/components/ftel_isp/stats/af/ftel_isp_af.c` | `f4b6185d959ab6f04c58e146d9fc4a949a8b71579cf626c9d6ecbb196804ca0c` |
| `FX1_ISP/algorithms/ae/ftel_isp_ae.c` | `neon-mdf/components/ftel_isp/algorithms/ae/ftel_isp_ae.c` | `575215a201d2978747dd3d39e575215d644cd673dec2c0dc756caa33747a2b28` |
| `FX1_ISP/algorithms/ae/ftel_isp_ae.h` | `neon-mdf/components/ftel_isp/algorithms/ae/ftel_isp_ae.h` | `17d92c55f7665099386deda8a689001df660624a87aedc793a1a38d4229a0396` |
| `FX1_ISP/io/output/ftel_isp_output.h` | `neon-mdf/components/ftel_isp/io/output/ftel_isp_output.h` | `cdeb2bee9cedc345bc0cfedba6225ed60458b5405c0e0ec0ca59ceaa139ae1b5` |
| `FX1_ISP/io/output/ftel_isp_output.c` | `neon-mdf/components/ftel_isp/io/output/ftel_isp_output.c` | `0180069d0dedd4284656ef863f0e0ab26ac3fce15d5946ff1c607b2107e4062e` |
| `FX1_ISP/io/dma/ftel_isp_dma.c` | `neon-mdf/components/ftel_isp/io/dma/ftel_isp_dma.c` | `1710918f92245d11b70a58acb4377195baf1df8c5d599fa0bdd32affc13152f7` |
| `FX1_ISP/io/dma/ftel_isp_dma.h` | `neon-mdf/components/ftel_isp/io/dma/ftel_isp_dma.h` | `786b3c532681dba8ad53031ba6481df5c249af90dd6fc0aa5b1db16fd7f9d0e7` |
| `FX1_ISP/registers/ftel_isp_regs.h` | `neon-mdf/components/ftel_isp/registers/ftel_isp_regs.h` | `128919b5b0d87bc1c9eaa146bb7cad8b9065d13aeb8cf713a1ff39381d7d343a` |
| `FX1_ISP/registers/regmap-v1.07.h` | `neon-mdf/components/ftel_isp/registers/regmap-v1.07.h` | `44c726159a5f5ff05f6ef5eb913a760078d0991bbffce2d799737ba963e831e3` |
| `FX1_ISP/tests/lsc/main_init_vp.c` | `examples/FX1_ISP_Init/Src/main.c` | `43cb7fb9da97605da2bb07a8d21cf2f7f57665b28841da29ba3c7d7a52f4de81` |
| `FX1_ISP/tests/init/main_full_vp.c` | `examples/FX1_ISP_FullInit/Src/main.c` | `d0680d9f1288159c15868264010e3930b4c0e37903d8667245c974437256c6b8` |
| `FX1_ISP/tests/init/full_init_check.c` | `examples/FX1_ISP_FullInit/Src/full_init_check.c` | `06273ddd31d7f2276431bfe6fa4638e646a26b1399e8314a199160d2cd603fa0` |
| `FX1_ISP/tests/init/full_init_check.h` | `examples/FX1_ISP_FullInit/Src/full_init_check.h` | `065007227f5a5caf3cdb2cf79aa064fb49bc0991cdb722b1074fa7eea478208a` |

Additional example imports: `FX1_ISP/tests/init/init_check.h` for FullInit; Frame derives from `examples/baremetal/frame_vp.c` and `tests/frame/frame_fixture.h`, adding local RAW/guard generation and DMA ownership fences so no host fixture loader is needed.
