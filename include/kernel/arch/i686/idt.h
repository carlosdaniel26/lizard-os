#ifdef TARGET_I686
#ifndef IDT_H
#define IDT_H
#include <stdint.h>

typedef struct interrupt_descriptor {
	uint16_t base_low;  /* ISR's address base low part*/
	uint16_t selector;  /* GDT segment that the CPU will load into CS before calling the ISR*/
	uint8_t always0;
	uint8_t flags;	  /* attributes*/
	uint16_t base_high; /* the higher 16 bits of the ISR's address*/
}__attribute__((packed)) interrupt_descriptor;

typedef struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_ptr;

void init_idt(void);
interrupt_descriptor create_idt_descriptor(void (*isr)(), uint8_t flags);







/* stubs */
extern void stub_0();
extern void stub_1();
extern void stub_2();
extern void stub_3();
extern void stub_4();
extern void stub_5();
extern void stub_6();
extern void stub_7();
extern void stub_8();
extern void stub_9();
extern void stub_10();
extern void stub_11();
extern void stub_12();
extern void stub_13();
extern void stub_14();
extern void stub_15();
extern void stub_16();
extern void stub_17();
extern void stub_18();
extern void stub_19();
extern void stub_20();
extern void stub_21();
extern void stub_22();
extern void stub_23();
extern void stub_24();
extern void stub_25();
extern void stub_26();
extern void stub_27();
extern void stub_28();
extern void stub_29();
extern void stub_30();
extern void stub_31();
extern void stub_32();
extern void stub_33();
extern void stub_34();
extern void stub_35();
extern void stub_36();
extern void stub_37();
extern void stub_38();
extern void stub_39();
extern void stub_40();
extern void stub_41();
extern void stub_42();
extern void stub_43();
extern void stub_44();
extern void stub_45();
extern void stub_46();
extern void stub_47();
extern void stub_48();
extern void stub_49();
extern void stub_50();
extern void stub_51();
extern void stub_52();
extern void stub_53();
extern void stub_54();
extern void stub_55();
extern void stub_56();
extern void stub_57();
extern void stub_58();
extern void stub_59();
extern void stub_60();
extern void stub_61();
extern void stub_62();
extern void stub_63();
extern void stub_64();
extern void stub_65();
extern void stub_66();
extern void stub_67();
extern void stub_68();
extern void stub_69();
extern void stub_70();
extern void stub_71();
extern void stub_72();
extern void stub_73();
extern void stub_74();
extern void stub_75();
extern void stub_76();
extern void stub_77();
extern void stub_78();
extern void stub_79();
extern void stub_80();
extern void stub_81();
extern void stub_82();
extern void stub_83();
extern void stub_84();
extern void stub_85();
extern void stub_86();
extern void stub_87();
extern void stub_88();
extern void stub_89();
extern void stub_90();
extern void stub_91();
extern void stub_92();
extern void stub_93();
extern void stub_94();
extern void stub_95();
extern void stub_96();
extern void stub_97();
extern void stub_98();
extern void stub_99();
extern void stub_100();
extern void stub_101();
extern void stub_102();
extern void stub_103();
extern void stub_104();
extern void stub_105();
extern void stub_106();
extern void stub_107();
extern void stub_108();
extern void stub_109();
extern void stub_110();
extern void stub_111();
extern void stub_112();
extern void stub_113();
extern void stub_114();
extern void stub_115();
extern void stub_116();
extern void stub_117();
extern void stub_118();
extern void stub_119();
extern void stub_120();
extern void stub_121();
extern void stub_122();
extern void stub_123();
extern void stub_124();
extern void stub_125();
extern void stub_126();
extern void stub_127();
extern void stub_128();
extern void stub_129();
extern void stub_130();
extern void stub_131();
extern void stub_132();
extern void stub_133();
extern void stub_134();
extern void stub_135();
extern void stub_136();
extern void stub_137();
extern void stub_138();
extern void stub_139();
extern void stub_140();
extern void stub_141();
extern void stub_142();
extern void stub_143();
extern void stub_144();
extern void stub_145();
extern void stub_146();
extern void stub_147();
extern void stub_148();
extern void stub_149();
extern void stub_150();
extern void stub_151();
extern void stub_152();
extern void stub_153();
extern void stub_154();
extern void stub_155();
extern void stub_156();
extern void stub_157();
extern void stub_158();
extern void stub_159();
extern void stub_160();
extern void stub_161();
extern void stub_162();
extern void stub_163();
extern void stub_164();
extern void stub_165();
extern void stub_166();
extern void stub_167();
extern void stub_168();
extern void stub_169();
extern void stub_170();
extern void stub_171();
extern void stub_172();
extern void stub_173();
extern void stub_174();
extern void stub_175();
extern void stub_176();
extern void stub_177();
extern void stub_178();
extern void stub_179();
extern void stub_180();
extern void stub_181();
extern void stub_182();
extern void stub_183();
extern void stub_184();
extern void stub_185();
extern void stub_186();
extern void stub_187();
extern void stub_188();
extern void stub_189();
extern void stub_190();
extern void stub_191();
extern void stub_192();
extern void stub_193();
extern void stub_194();
extern void stub_195();
extern void stub_196();
extern void stub_197();
extern void stub_198();
extern void stub_199();
extern void stub_200();
extern void stub_201();
extern void stub_202();
extern void stub_203();
extern void stub_204();
extern void stub_205();
extern void stub_206();
extern void stub_207();
extern void stub_208();
extern void stub_209();
extern void stub_210();
extern void stub_211();
extern void stub_212();
extern void stub_213();
extern void stub_214();
extern void stub_215();
extern void stub_216();
extern void stub_217();
extern void stub_218();
extern void stub_219();
extern void stub_220();
extern void stub_221();
extern void stub_222();
extern void stub_223();
extern void stub_224();
extern void stub_225();
extern void stub_226();
extern void stub_227();
extern void stub_228();
extern void stub_229();
extern void stub_230();
extern void stub_231();
extern void stub_232();
extern void stub_233();
extern void stub_234();
extern void stub_235();
extern void stub_236();
extern void stub_237();
extern void stub_238();
extern void stub_239();
extern void stub_240();
extern void stub_241();
extern void stub_242();
extern void stub_243();
extern void stub_244();
extern void stub_245();
extern void stub_246();
extern void stub_247();
extern void stub_248();
extern void stub_249();
extern void stub_250();
extern void stub_251();
extern void stub_252();
extern void stub_253();
extern void stub_254();
extern void stub_255();

#endif
#endif