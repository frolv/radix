/*
 * arch/i386/cpu/isr.h
 * Copyright (C) 2016-2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ARCH_I386_ISR_H
#define ARCH_I386_ISR_H

#define NUM_ISR_VECTORS 256
#define NUM_EXCEPTIONS  32
#define IRQ_BASE        0x20
#define ISA_IRQ_COUNT   16

void early_isr_0(void);
void early_isr_1(void);
void early_isr_2(void);
void early_isr_3(void);
void early_isr_4(void);
void early_isr_5(void);
void early_isr_6(void);
void early_isr_7(void);
void early_isr_8(void);
void early_isr_9(void);
void early_isr_10(void);
void early_isr_11(void);
void early_isr_12(void);
void early_isr_13(void);
void early_isr_14(void);
void early_isr_15(void);
void early_isr_16(void);
void early_isr_17(void);
void early_isr_18(void);
void early_isr_19(void);
void early_isr_20(void);
void early_isr_21(void);
void early_isr_22(void);
void early_isr_23(void);
void early_isr_24(void);
void early_isr_25(void);
void early_isr_26(void);
void early_isr_27(void);
void early_isr_28(void);
void early_isr_29(void);
void early_isr_30(void);
void early_isr_31(void);

void isr_0(void);
void isr_1(void);
void isr_2(void);
void isr_3(void);
void isr_4(void);
void isr_5(void);
void isr_6(void);
void isr_7(void);
void isr_8(void);
void isr_9(void);
void isr_10(void);
void isr_11(void);
void isr_12(void);
void isr_13(void);
void isr_14(void);
void isr_15(void);
void isr_16(void);
void isr_17(void);
void isr_18(void);
void isr_19(void);
void isr_20(void);
void isr_21(void);
void isr_22(void);
void isr_23(void);
void isr_24(void);
void isr_25(void);
void isr_26(void);
void isr_27(void);
void isr_28(void);
void isr_29(void);
void isr_30(void);
void isr_31(void);
void isr_32(void);
void isr_33(void);
void isr_34(void);
void isr_35(void);
void isr_36(void);
void isr_37(void);
void isr_38(void);
void isr_39(void);
void isr_40(void);
void isr_41(void);
void isr_42(void);
void isr_43(void);
void isr_44(void);
void isr_45(void);
void isr_46(void);
void isr_47(void);
void isr_48(void);
void isr_49(void);
void isr_50(void);
void isr_51(void);
void isr_52(void);
void isr_53(void);
void isr_54(void);
void isr_55(void);
void isr_56(void);
void isr_57(void);
void isr_58(void);
void isr_59(void);
void isr_60(void);
void isr_61(void);
void isr_62(void);
void isr_63(void);
void isr_64(void);
void isr_65(void);
void isr_66(void);
void isr_67(void);
void isr_68(void);
void isr_69(void);
void isr_70(void);
void isr_71(void);
void isr_72(void);
void isr_73(void);
void isr_74(void);
void isr_75(void);
void isr_76(void);
void isr_77(void);
void isr_78(void);
void isr_79(void);
void isr_80(void);
void isr_81(void);
void isr_82(void);
void isr_83(void);
void isr_84(void);
void isr_85(void);
void isr_86(void);
void isr_87(void);
void isr_88(void);
void isr_89(void);
void isr_90(void);
void isr_91(void);
void isr_92(void);
void isr_93(void);
void isr_94(void);
void isr_95(void);
void isr_96(void);
void isr_97(void);
void isr_98(void);
void isr_99(void);
void isr_100(void);
void isr_101(void);
void isr_102(void);
void isr_103(void);
void isr_104(void);
void isr_105(void);
void isr_106(void);
void isr_107(void);
void isr_108(void);
void isr_109(void);
void isr_110(void);
void isr_111(void);
void isr_112(void);
void isr_113(void);
void isr_114(void);
void isr_115(void);
void isr_116(void);
void isr_117(void);
void isr_118(void);
void isr_119(void);
void isr_120(void);
void isr_121(void);
void isr_122(void);
void isr_123(void);
void isr_124(void);
void isr_125(void);
void isr_126(void);
void isr_127(void);
void isr_128(void);
void isr_129(void);
void isr_130(void);
void isr_131(void);
void isr_132(void);
void isr_133(void);
void isr_134(void);
void isr_135(void);
void isr_136(void);
void isr_137(void);
void isr_138(void);
void isr_139(void);
void isr_140(void);
void isr_141(void);
void isr_142(void);
void isr_143(void);
void isr_144(void);
void isr_145(void);
void isr_146(void);
void isr_147(void);
void isr_148(void);
void isr_149(void);
void isr_150(void);
void isr_151(void);
void isr_152(void);
void isr_153(void);
void isr_154(void);
void isr_155(void);
void isr_156(void);
void isr_157(void);
void isr_158(void);
void isr_159(void);
void isr_160(void);
void isr_161(void);
void isr_162(void);
void isr_163(void);
void isr_164(void);
void isr_165(void);
void isr_166(void);
void isr_167(void);
void isr_168(void);
void isr_169(void);
void isr_170(void);
void isr_171(void);
void isr_172(void);
void isr_173(void);
void isr_174(void);
void isr_175(void);
void isr_176(void);
void isr_177(void);
void isr_178(void);
void isr_179(void);
void isr_180(void);
void isr_181(void);
void isr_182(void);
void isr_183(void);
void isr_184(void);
void isr_185(void);
void isr_186(void);
void isr_187(void);
void isr_188(void);
void isr_189(void);
void isr_190(void);
void isr_191(void);
void isr_192(void);
void isr_193(void);
void isr_194(void);
void isr_195(void);
void isr_196(void);
void isr_197(void);
void isr_198(void);
void isr_199(void);
void isr_200(void);
void isr_201(void);
void isr_202(void);
void isr_203(void);
void isr_204(void);
void isr_205(void);
void isr_206(void);
void isr_207(void);
void isr_208(void);
void isr_209(void);
void isr_210(void);
void isr_211(void);
void isr_212(void);
void isr_213(void);
void isr_214(void);
void isr_215(void);
void isr_216(void);
void isr_217(void);
void isr_218(void);
void isr_219(void);
void isr_220(void);
void isr_221(void);
void isr_222(void);
void isr_223(void);
void isr_224(void);
void isr_225(void);
void isr_226(void);
void isr_227(void);
void isr_228(void);
void isr_229(void);
void isr_230(void);
void isr_231(void);
void isr_232(void);
void isr_233(void);
void isr_234(void);
void isr_235(void);
void isr_236(void);
void isr_237(void);
void isr_238(void);
void isr_239(void);
void isr_240(void);
void isr_241(void);
void isr_242(void);
void isr_243(void);
void isr_244(void);
void isr_245(void);
void isr_246(void);
void isr_247(void);
void isr_248(void);
void isr_249(void);
void isr_250(void);
void isr_251(void);
void isr_252(void);
void isr_253(void);
void isr_254(void);
void isr_255(void);

void load_interrupt_routines(void);

#endif /* ARCH_I386_ISR_H */
