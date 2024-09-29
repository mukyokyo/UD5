/*!
 @file  sample13_DAC_SYNTH.cpp
 @brief Wavetableシンセとミュージックシーケンサ
 @note
  取り込んだRAWファイルを元にGPIO7からアナログ波形を出力。
  スピーカーアンプをつなげば音となる。
  簡易的なMMLで音階を奏でられる。
 */


#include <ud5.h>

CDXIF dx;

#define _GRA2MSX              //!< 必要に応じてコメントを外す

#ifdef _GRA2MSX
INCBIN ("kick.raw", pcm_kick);
INCBIN ("sd.raw", pcm_sn);
INCBIN ("hh2.raw", pcm_hh);
INCBIN ("cc.raw", pcm_cc);
INCBIN ("tom.raw", pcm_tom);
//INCBIN ("hit2.raw", pcm_hit);
#endif


std::vector<CPCM::TPCMInfo> PCMInfo{
#ifdef _GRA2MSX
  { (int8_t *)&pcm_kick, (int)&_size_pcm_kick, -1, 256, 127 },
  { (int8_t *)&pcm_sn,   (int)&_size_pcm_sn,   -1, 256, 127 },
  { (int8_t *)&pcm_hh,   (int)&_size_pcm_hh,   -1, 256, 127 },
  { (int8_t *)&pcm_cc,   (int)&_size_pcm_cc,   -1, 256, 127 },
  { (int8_t *)&pcm_tom,  (int)&_size_pcm_tom,  -1, 256, 127 },
//  { (int8_t *)&pcm_hit,  (int)&_size_pcm_hit,  -1, 256, 127 },
#else
#endif
};

CPCM PCM (&PCMInfo, 32);

CTinyMusicSq MSq[_MAX_MUSICSQ];


// tasks
//-----------------------------
//! 再生停止
static void StopMusic (void) {
  for (int i = 0; i < _MAX_MUSICSQ; i++) MSq[i].StopMusic ();
}

//! MML解析と再生
bool ConvertAndStartMusic (uint8_t num, ...) {
  bool result = true;
  if (num > _MAX_MUSICSQ) return false;
  StopMusic ();
  va_list ap;
  va_start (ap, num);
  for (int i = 0; i < num && result; i++)
    if (!MSq[i].Convert (va_arg(ap, const char *))) result = false;
  va_end(ap);
  for (int i = 0; i < num; i++) MSq[i].StartMusic ();
  return result;
}

//! キー入力タスク
void KeyTask (void *pvParameters) {
  for (;;) {
    while (dx.rxbuff ()) {
      char ch = dx.getc ();
      switch (ch) {

#ifndef _GRA2MSX
        // 入力文字に応じて何かしら演奏開始
        // テンポは分解能の都合でずれがある
        // スタッカートはEGで強引に対処
        case 'z':
          SetWaveForm (0, (const int8_t[]){0,48,80,80,112,112,80,80,112,112,80,32,0,0,-32,-32,0,32,32,0,0,-32,-80,-112,-112,-80,-80,-112,-112,-80,-80,-64},32);
          SetWaveForm (1, (const int8_t[]){0,48,80,96,112,96,80,48,0,-48,-80,-96,-112,-96,-80,-48,0,64,96,112,96,64,0,-64,-96,-112,-96,-64,0,112,0,-112}, 32);
          ConvertAndStartMusic (4,
            "t6@1o2v40l16 b-6<f12b-4 a-4gfeg f6e12f2e-6f12 <c4>b-4a-8g-8f8e-8 f2."
            ,
            "t6@1o3v40l16 r6d12f4e-4ddce- d6c12d2c6d12 a-4g4f8e-8d8c8 d2."
            ,
            "t6@1o3v40l16 r6r12d4c4>b-b-a-b- b-6a-12b-2a-6b-12 <e-4e-4d-8c8>b-8a-8 b-2."
            ,
            "t6@0o1v64l16$,,100, b-1& r2 b-8 f32&r64r64 f32&r64r64 b-db-d b-1 b-4f4>b-4"
          );
          break;
        case 'x':
          SetWaveForm (0, (const int8_t[]){-104,-88,-72,-56,-40,-24,-8,-8,24,40,56,72,88,104,120,-120}, 16);
          SetWaveForm (1, (const int8_t[]){-104,-88,-72,-56,-40,-24,-8,-8,24,40,56,72,88,104,120,120,104,88,72,56,40,24,-8,-8,-24,-40,-56,-72,-88,-104,-120,-120}, 32);
          SetWaveForm (2, (const int8_t[]){110,-110}, 2);
          ConvertAndStartMusic (3,
            "t6@0$,100,100,l24 o4d8eg8b6ge6d8eg8a+6ge6d8eg8b6gb8<cd8c>a+8ga+8gd64"
            ,
            "t6@1$,100,100,l24 o4d4&r>b6g8&r12<d4&r>a+6g8&r12<d4&r>b6g6<d>a+8gf+8df+8g"
            ,
            "t6@2$,100,100,l24 o2[6g6<g6>]c8<c24>d8<d24>g8<g24>g64"
          );
          break;
        case 'c':
          SetWaveForm (0, (const int8_t[]){-100,100}, 2);
          ConvertAndStartMusic (2,
            "t6@0$,100,100, [o3l8, a16&r32&r64r64af+ear<c4> a16&r32&r64r64af+earf+4 a16&r32&r64r64af+ea<cdd+ el16d+ed+ed+e32r32e8r8 edc>bl8a16&r32&r64r64a f+ear<c4> a16&r32&r64r64af+ear f+4 a16&r32&r64r64af+ea<cdd+ ae16d+16c>a gg+ar"
            "[2o3 a64<e4 c+e f+c+ef+ c+>baf+a2 f+aba <c+>bab <c+>a4f+ e2]]"
            ,
            "t6@0$,100,100,l12 [o1 [6ar24ar24<ar24ar24>] er24er24<er24er24 er24>er24f+r24g+r24 [6ar24ar24<ar24ar24>] dr24<dr24>er24<er24>ar24<ar24ar24rr24"
            "o1 [2 r64 [4ar24ar24<ar24ar24>] [2dr24dr24<dr24dr24>] [2ar24ar24<ar24ar24>]] ]"
          );
          break;
        case 'v':
          SetWaveForm (0, (const int8_t[]){-100,100}, 2);
          ConvertAndStartMusic (2,
            "t6@0$,100,100,0 [o3l8 [2agea g+ed+e d+c>a<d c+>a4. <dc>ag a<cdd+ eged c+2]"
            "o4 e4 c+e f+c+ef+ c+>baf+a2 f+aba <c+>bab <c+>a4f+ e2 a64<e4c+e f+c+ef+ c+>baf+a2 f+aba <c+>bab <c+>a4f+ e2]"
            ,
            "t6@0$,100,100,0 [o1l16 [4arar<arar>] [2drdr<drdr>] [6arar<arar>] [2drdr<drdr>] [2arar<arar>]"
            "o1l12 [4ar24ar24<ar24ar24>] [2dr24dr24<dr24dr24>] [2ar24ar24<ar24ar24>] r64 [4ar24ar24<ar24ar24>] [2dr24dr24<dr24dr24>] [2ar24ar24<ar24ar24>]]"
          );
          break;
        case 'b':
          SetWaveForm (0, (const int8_t[]){-80,-112,-16,96,64,16,64,96,32,-16,64,112,80,0,32,48,-16,-96,0,80,16,-64,-48,-16,-96,-128,-80,0,-48,-112,-80,-32}, 32);
          SetWaveForm (1, (const int8_t[]){100,-100}, 2);
          ConvertAndStartMusic (5,
            "t2@0o2$0,11,65,7 d4 [l8o2 gabg<c4.>b16a16 bgeaf+f+16e16d4 gabg<c4.>a b<c>aa16g16g2"
            "o2 rbegc4rd16e16 f+gac>b4r<c16d16 ef+g>b<c4g4 rf+16e16f+f+16g16g2&"
            "o2 ref+ga2& rf+gab2& r<cd>b<c4.>b ab<c>ab4.a gcegf+gaf+ gf+edc+4.d d1&r2."
            "l16o1 a<deab8g8e8d8c+4 >a<ceaa8f+8d8c8>b4 ab<dgg8e8c+8>b8a+4 &r8<c+8>b4.<c+dc+4&"
            "l8o2 rf+16c+16>b4<g4.e f+g>a4<a4.d ef+>gb<ef+g4& rc+d2.&r4 dr]"
            ,
            "t2@1$0,11,65,7 r4 [l8o4 d1 r4>a4a4r<c >b4<d4g2 &r4r2."
            "r1r1r1r1"
            "o4 c2.re d+2rf+re& rrf4e4.r d1"
            "o4 e>a<ced4.c >b2.ag f+16r16l32f+rgrargrf+rar l16grara+rgr f+2r4"
            "l8o3 r2e4e4 r2d4d4 r2c+rc+4 r1"
            "o3 e4.rr2 r4c+2r4 r2r4r>b r1 r2]"
            ,
            "t2@1v50$0,11,65,7l8 r4 [o4 r1r1r2cr4>c <d.r16c.r16r>b16a16b4"
            "o3 b2r4ag a2r4gf+ g2rb<c>b16<c16 drcr>b4.r"
            "o3 arrgf+gaf+ brragaba g+rb4ra16g16ag f+ra4rg16f+16gf+"
            "o3 e4.rdef+<d >g2.f+e dd16e16f+16e16d16f+16ef+ge f+2r4"
            "o3 r2b2 r2a2 r2grf+4 &r2r4f+4"
            "o3 a4.rgf+e4 d4c+2r4 d2.r4 c+grf+egf+4 r2]"
            ,
            "t2@1k10v30$0,11,65,7l8 r4 [o4 r1r1r2cr4>c <d.r16c.r16r>b16a16b4"
            "o3 b2r4ag a2r4gf+ g2rb<c>b16<c16 drcr>b4.r"
            "o3 arrgf+gaf+ brragaba g+rb4ra16g16ag f+ra4rg16f+16gf+"
            "o3 e4.rdef+<d >g2.f+e dd16e16f+16e16d16f+16ef+ge f+2r4"
            "o3 r2b2 r2a2 r2grf+4 &r2r4f+4"
            "o3 a4.rgf+e4 d4c+2r4 d2.r4 c+grf+egf+4 r2]"
            ,
            "t2@0$0,11,65,7l8 r4 [o1 b4g4a4f+4 g4c+4def+e g4f4.e16d16ec def+eg>g<gf+"
            "o2 e4g4a4>a4 <d4f+4g4>g4 <c4e4>ab<c>a <d4rdef+ge"
            "o1 a1 a1 a1 a1 a1"
            "o1 g4b4a4.r d1d2."
            "o1 r2g+4g+4 r2f+4g4 r2d+4f+4 <e4d4>b4a4"
            "o1 f+4g2.& r4g4f+ef+4& r4e2a4& r4<d2de f+ddr]"
          );
          break;
        case 'n':
          SetWaveForm (0, (const int8_t[]){-120,-104,-88,-72,-56,-40,-24,-8,24,40,56,72,88,104,120}, 16);
          SetWaveForm (1, (const int8_t[]){-128,-96,-80,-64,-32,-32,-64,-48,16,32,32,0,32,-16,32,16,-16,-16,16,-32,0,-32,-48,-32,32,64,32,32,64,80,96,112}, 32);
          SetWaveForm (2, (const int8_t[]){-16,32,64,80,96,80,64,32,-16,-64,-96,-112,-128,-112,-96,-64,-16,48,80,96,80,48,-16,-80,-112,-128,-112,-80,-16,96,-16,-128}, 32);
          SetWaveForm (3, (const int8_t[]){-16,96,64,16,64,96,32,-16,64,112,80,0,32,48,-16,-96,0,80,16,-64,-48,-16,-96,-128,-80,0,-48,-112,-80,-32,-80,-112}, 32);
          SetWaveForm (4, (const int8_t[]){32,64,64,32,-16,-16,0,48,80,96,80,32,-32,-48,-48,-16,16,16,0,-64,-112,-128,-112,-80,-32,-16,-16,-64,-96,-96,-64,-16}, 32);
          ConvertAndStartMusic (4,
            "t8l16$,8,10, @2[o3"
            "[2d+4b4a+4b4<[7c32r32]>[9b32r32] a4b4a8.ge8.a32r32[8a32r32a32r32] r4a4g+4a4[7b32r32][9a32r32] g4a4g8.e4d32r32[8d32r32d32r32] >b8.<dg8.>b<d8.g>b8.<gc8.eg8.c+e8.gc+8.g r4b8.a+a8r8d8.g8.r8g8r8f+8r8e8r8]"
            ">b8.<f+d+8.>b<f+8.d+>b8.<f+ d+8.>b<f+8.d+>b8.<f+d+8.>b< f+8.d+>b8.<f+d+8.>b<f+8.d+ >b8.<f+d+8.>b<f+8.d+>b8.<f+"
            ">a8.<ec+8.>a<e8.c+>a8.<e c+8.>a<e8.c+>a8.<ec+8.>a< e8.c+>a8.<ec+8.>a<e8.c+ >a8.<ec+8.>a<e8.c+>a4<"
            ">g8.ab8.ga8.b<c8.>ab8.<cd8.>b<c8.de8.c d8.ef+8.de8.f+g8.ab8.ra+8.ba+8.ba+8.b"
            "d+4b4a+4b4<[7c32r32]>[9b32r32] a4b4a8.ge8.a32r32[8a32r32a32r32] r4a4g+4a4[7b32r32][9a32r32] g4a4g8.e4d32r32[8d32r32d32r32] >b8.<dg8.>b<d8.g>b8.<gc8.eg8.c+e8.gc+8.g r4b8.a+a8r8d8.g8.r8g8r8f+8r8e8r8"
            "r4b4a+4b4<c4>b4a+4b4 e4<f+4e4>b4g2.r4 r4a4g+4a4b4a4b4<c+4> d4<d4c4>b4a4r4a+2 r4b4a+4b4<c4>b4a+4b4 r4g4b4<d4f+8.ef+8.e&r8.ed+8.e r4g2e8.>b<d8.e>b8.<de8.>a+b8.g&r4f+4e4d4 g4r8g8r2"
            "d+4b4a+4b4<[7c32r32]>[9b32r32] a4b4a8.ge8.a32r32[8a32r32a32r32] r4a4g+4a4[7b32r32][9a32r32] g4a4g8.e4d32r32[8d32r32d32r32] >b8.<dg8.>b<d8.g>b8.<gc8.eg8.c+e8.gc+8.g g8.ab8.ge8r8d8r8r4f+8r8g8r8r4 >g8.ab8.ga8.b<c8.>ab8.<cd8.>b<c8.de8.c d8.ef+8.de8.f+g8.ab8.ra+8.ba+8.ba+8.b]"
            ,
            "t8l16 [@4$,16,40,o3"
            "[2 b2g+4e4b4&r8.<cd4c4> b4&r8.<c>b4a4g4&r8.f+e4d+4 d8r8a8.ba8.ge8r8a8.ba8.ge8r8d8.e g4a4b4<c4d4c4>b4a4 d8.>b<g8.d>b8.<gd8.ge8.cg8.ec+8.ge8.c+ r1r4d8.gc+8.gc8.g]"
            "@0$,30,30,o2 b2<d+4.f+8g+4b2g+4f+2g+2d+4f+2. >a2<c+4.e8f+4a2f+4e2f+2c+4e2."
            "@4$,16,40,o2 b8.<cd8.>b<c8.de8.cd8.ef+8.de8.f+g8.e f+8.ga8.f+g8.ab8.<c+d+8.rd8.d+d8.d+d8.d+"
            "o3b2g+4e4b4&r8.<cd4c4> b4&r8.<c>b4a4g4&r8.f+e4d+4 d8r8a8.ba8.ge8r8a8.ba8.ge8r8d8.e g4a4b4<c4d4c4>b4a4 d8.>b<g8.d>b8.<gd8.ge8.cg8.ec+8.ge8.c+ r1r4d8.gc+8.gc8.g"
            "r4@3 f+4r8f+4r8f+4r8f+4r8f+4 r4e4r8e4r8e4r8e4r8e4 r4e4r8e4r8e4r8e4r8e4 r4f+4r8f+4r8f+4r4f+4r4"
            "r4f+4r8f+4r8f+4r8f+4r8f+4 r4g4r8g4r8r4g+4r8g+4r8 r4e4r4e4r4d4r4d4 r4d4r8d4r8e4r8e8r4r4"
            "@4 b2g+4e4b4&r8.<cd4c4> b4&r8.<c>b4a4g4&r8.f+e4d+4 d8r8a8.ba8.ge8r8a8.ba8.ge8r8d8.e g4a4b4<c4d4c4>b4a4 d8.>b<g8.d>b8.<gd8.ge8.cg8.ec+8.ge8.c+ r1r4a8r8g8r8r4"
            ">b8.<cd8.>b<c8.de8.cd8.ef+8.de8.f+g8.e f+8.ga8.f+g8.ab8.<c+d+8.rd8.d+d8.d+d8.d+>]"
            ,
            "t8l16$,16,64,@3 [o3"
            "[6r1r1r1r1][6r1r1r1r1]"
            "r4d+4r8d+4r8d+4r8d+4r8d+4> r4b4r8b4r8b4r8b4r8b4 r4a4r8a4r8a4r8a4r8a4< r4d4r8d4r8d4r4d4r4"
            "r4d+4r8d+4r8d+4r8d+4r8d+4 r4d4r8d4r8r4d4r8d4r8 r4c4r4c4>r4a4r4a4 r4b4r8b4r8b4r8b8r4r4<"
            "[4r1r1r1r1]]"
            ,
            "t8l16@1$,12,30, [o2"
            "[2 e4r4>b4<r8re4e>b4<e8.f+g+4 a4r4e4r8ra4ag4f+4e4 d4r4>a4<r8rd4d>a4<d8.ef+4 g4r4d4r8rg4gd4c+4c4> g4<g4>g4<g4>a4<a4>a+4<a+4> b4r4r4<d8.g8.r8g8r8a8r8a+8r8 ]"
            "b4r4f+4r8rb4ba+4g+4f+4 b4<c+4d+4f+4b8.ba+4g+4f+4> a4r4e4r8ra4ag+4f+4e4 a4b4<c+4e4a8.ag+4f+4e4> >g4<g4>a4<a4>b4<b4c4<c4 >d4<d4>e4<e4>f+4f+4d+4>b4<"
            "e4r4>b4<r8re4e>b4<e8.f+g+4 a4r4e4r8ra4ag4f+4e4 d4r4>a4<r8rd4d>a4<d8.ef+4 g4r4d4r8rg4gd4c+4c4> g4<g4>g4<g4>a4<a4>a+4<a+4> b4r4r4<d8.g8.r8g8r8a8r8a+8r8"
            "b4r4f+4r8rb4bf+4b8.<c+d+4 e4r4>b4<r8re4ed4c+8.c+>b4 a4r4e4r8ra4ae4a8.b<c+4 d4r4r2r2d2>"
            "b4r4f+4r8rb4b<c4>b8.ba4 g4r4b4<d4e4g+2e4 c2c2d2d2> g4f+4e4d4g4r8g8r2"
            "e4r4>b4<r8re4e>b4<e8.f+g+4 a4r4e4r8ra4ag4f+4e4 d4r4>a4<r8rd4d>a4<d8.ef+4 g4r4d4r8rg4gd4c+4c4> g4<g4>g4<g4>a4<a4>a+4<a+4 g8.ab8.ge4d4 d8.ef+4g4r4"
            ">g4<g4>a4<a4>b4<b4c4<c4 >d4<d4>e4<e4>f+4f+4d+4>b4]"
          );
          break;
        case 'm':
          SetWaveForm (0, (const int8_t[]){-120,-120,-104,-104,-88,-88,-72,-72,-56,-56,-40,-40,-24,-24,-8,-8,-8,-8,24,24,40,40,56,56,72,72,88,88,104,104,120,120}, 32);
          SetWaveForm (1, (const int8_t[]){-120,-104,-88,-72,-56,-40,-24,-8,-8,24,40,56,72,88,104,120,-120,-104,-88,-72,-56,-40,-24,-8,-8,24,40,56,72,88,104,120}, 32);
          SetWaveForm (2, (const int8_t[]){56,88,104,88,72,40,-8,-8,-8,40,72,88,104,88,56,-8,-56,-88,-104,-88,-72,-40,-8,-8,-8,-40,-72,-88,-104,-88,-56,-8}, 32);
          SetWaveForm (3, (const int8_t[]){-8,104,72,24,72,104,40,-8,72,120,88,-8,40,56,-8,-88,-8,88,24,-56,-40,-8,-88,-120,-72,-8,-40,-104,-72,-24,-72,-104}, 32);
          ConvertAndStartMusic (4,
            "t5$,13,10,@0o4l24 f6&rr16rc16&ra16&r<c4&r16 d6&rr16r>a6&rg16&rf6&r   d6&rf6&rf16&rd16&ra16&rg4&r16r2r8"
            "o4 f6&rr16rc16&ra16&r<c4&r16 d6&rr16r>a6&rg16&rf6&r  d6&rf6&rd16&rf6&rc4&r16r2r8"
            "o4 r6rd16&rf16&rf16&rd16&rf16&rg16&re16&rc6&rd4&r16r6r d6&rf6&rf16&rd16&rf16&rc4&r16r2r8"
            "o4 r6rd16&rf16&rf16&rd16&rf16&rg16&ra16&rg6&rf4&r16r6r g6&rg6&rg16&ra6&r<$,33,50,c2&r4&r8&r16$,13,10,"
            "o5 d6&rr16r>a16&r<c16&r d4&r16c6&rr16r>a6&rg16&rf6&r d6&rf6&rf16&rd16&ra16&rg4&r16r2r8"
            "o4 f6&rr16rc16&ra16&r<c4&r16 d6&rr16r>a6&rg16&rf6&r  d6&rf6&rf16&rd16&ra16&rg4&r16r2r8"
            "o4 r6rd16&rf16&rf16&rd16&rf16&rg16&ra16&r<c6&r d6&r>a4&r16 g6&rg6&r<c16&r>a4&r16f6&rr6r@1f6&rr6r"
            ,
            "t5$,13,10,@0o4l24 k6v30 f6&rr16rc16&ra16&r<c4&r16 d6&rr16r>a6&rg16&rf6&r   d6&rf6&rf16&rd16&ra16&rg4&r16r2r8"
            "o4 f6&rr16rc16&ra16&r<c4&r16 d6&rr16r>a6&rg16&rf6&r  d6&rf6&rd16&rf6&rc4&r16r2r8"
            "o4 r6rd16&rf16&rf16&rd16&rf16&rg16&re16&rc6&rd4&r16r6r d6&rf6&rf16&rd16&rf16&rc4&r16r2r8"
            "o4 r6rd16&rf16&rf16&rd16&rf16&rg16&ra16&rg6&rf4&r16r6r g6&rg6&rg16&ra6&r<$,33,50,c2&r4&r8&r16$,13,10,"
            "o5 d6&rr16r>a16&r<c16&r d4&r16c6&rr16r>a6&rg16&rf6&r d6&rf6&rf16&rd16&ra16&rg4&r16r2r8"
            "o4 f6&rr16rc16&ra16&r<c4&r16 d6&rr16r>a6&rg16&rf6&r  d6&rf6&rf16&rd16&ra16&rg4&r16r2r8"
            "o4 r6rd16&rf16&rf16&rd16&rf16&rg16&ra16&r<c6&r d6&r>a4&r16 g6&rg6&r<c16&r>a4&r16f6&rr6r@1f6&rr6r"
            ,
            "t5$,5,20,@2o3l24 f16&rf32&r48a32&r48<c16&r>a16&r<f16&rc16&r>a16&rf16&rd16&rd32&r48f32&r48a16&rf16&r<d16&r>a16&rf16&rd16&r"
            "o3 d16&rd32&r48f32&r48a+16&rf16&r<d16&r>a+16&rf16&rd16&rc16&rc32&r48e32&r48g16&re16&r<c16&r>g16&re16&rc16&r"
            "o3 f16&rf32&r48a32&r48<c16&r>a16&r<f16&rc16&r>a16&rf16&rd16&rd32&r48f32&r48a16&rf16&r<d16&r>a16&rf16&rd16&r"
            "[2o3 d16&rd32&r48f32&r48a+16&rf16&r<d16&r>a+16&rf16&rd16&rc16&rc32&r48e32&r48g16&re16&r<c16&r>g16&re16&rc16&r"
            "o3 d16&rd32&r48f32&r48a+16&rf16&r<d16&r>a+16&rf16&rd16&r>a16&ra+16&r<c16&rd6&re16&rf6&r]"
            "o3 d16&rd32&r48g32&r48b16&rg16&r<d16&r>b16&rg16&rd16&rc16&rc32&r48e32&r48g16&re16&r<c16&r>g16&re16&rc16&r"
            "[2o3 f16&rf32&r48a32&r48<c16&r>a16&r<f16&rc16&r>a16&rf16&rd16&rd32&r48f32&r48a16&rf16&r<d16&r>a16&rf16&rd16&r"
            "o3 d16&rd32&r48f32&r48a+16&rf16&r<d16&r>a+16&rf16&rd16&rc16&rc32&r48e32&r48g16&re16&r<c16&r>g16&re16&rc16&r]"
            "o3 d16&rd32&r48f32&r48a+16&rf16&r<d16&r>a+16&rf16&rd16&r>a16&ra+16&r<c16&rd6&re16&rf6&r"
            "o3 d16&rd32&r48f32&r48a+16&rf16&rc16&rc32&r48e32&r48g16&re16&rf6&rr6r<c6&rr6r"
            ,
            "t5$,11,22,@3l24"
            "[2o3 a6&ra16&rg16&ra16&rg16&ra16&r<c16&r>a6&ra16&rg16&ra16&rg16&rf16&rc16&r"
            "o3 a+6&ra+16&ra16&ra+16&ra16&rg16&rf16&re6&re16&rd16&re16&rd16&re16&rc16&r]"
            "o2 a+6&ra+16&ra16&ra+16&r<d16&rf16&rd16&rc16&rd16&re16&rf6&rg16&ra6&r"
            "o3 a+6&ra+16&ra16&ra+16&ra16&rg16&rf16&re6&re16&rd16&re16&rd16&re16&rc16&r"
            "o2 a+6&ra+16&ra16&ra+16&r<d16&rf16&rd16&rc16&rd16&re16&rf6&rg16&ra6&r"
            "o3 g6&rg16&rf+16&rg16&rf16&re16&rd16&rc16&rc16&rc+16&rc+16&rd16&rd16&re16&re16&r"
            "[2o3 a6&ra16&rg16&ra16&rg16&ra16&r<c16&r>a6&ra16&rg16&ra16&rg16&rf16&rc16&r"
            "o3 a+6&ra+16&ra16&ra+16&ra16&rg16&rf16&re6&re16&rd16&re16&rd16&re16&rc16&r]"
            "o2 a+6&ra+16&ra16&ra+16&r<d16&rf16&rd16&rc16&rd16&re16&rf6&rg16&ra6&r"
            "o3 g6&rg16&rf16&re16&rc16&rd16&re16&rf6&rr6ra6&rr6r"
          );
          break;
        case ',':
          SetWaveForm (0, (const int8_t[]){-120,-88,-72,-56,-24,-24,-56,-40,24,40,40,-8,40,-8,40,24,-8,-8,24,-24,-8,-24,-40,-24,40,72,40,40,72,88,104,120}, 32);
          SetWaveForm (1, (const int8_t[]){56,88,104,88,72,40,-8,-8,-8,40,72,88,104,88,56,-8,-56,-88,-104,-88,-72,-40,-8,-8,-8,-40,-72,-88,-104,-88,-56,-8}, 32);
          SetWaveForm (2, (const int8_t[]){-120,-104,-88,-72,-56,-40,-24,-8,-8,24,40,56,72,88,104,120}, 16);
          ConvertAndStartMusic (3,
            "t4@0o3l16$,5,15, agf+gbag+a<c>ba+b<dc>b<cfed+edc>bag8<g8$,15,32,g4"
            "o4$,5,15, dd+dc>a8g8r8<c8$,15,32,d+4$,5,15,dd+dc>a8g8r8<c8>$,15,32,a4<$,5,15,dd+dc>a8g8r8<cef8f+8   g>a<gec8>g8r<c>a8$,15,32,g4"
            "o4$,5,15, dd+dc>a8g8r8<c8$,15,32,d+4$,5,15,dd+dc>a8g8r8<c8>$,15,32,a4<$,5,15,dd+dc>a8g8r8<cef8f+8   $,3,5,g>a<g$,5,15,ec8>g8r16<d+16d8$,15,32,c4"
            "o3$,5,15, b<c>bag+8e8r8a8$,15,32,a4$,5,15,b<c>bag+8e8r8a8$,15,32,a4$,5,15, b<c>bag+8b8r8<c8$,15,32,c4 $,5,15,dd+dc>a8g+8g8<g8$,15,32,g4"
            "o4$,5,15, dd+dc>a8g8r8<c8$,15,32,d+4$,5,15,dd+dc>a8g8r8<c8>$,15,32,a4<$,5,15,dd+dc>a8g8r8<cef8f+8   g>a<gec8>g8r<d+d8$,15,32,c4"
            "r1r2r8$,15,32, @1o3b4."
            "@0o4$,5,15, dd+dc>a8g8r8<c8$,15,32,d+4$,5,15,dd+dc>a8g8r8<c8>$,15,32,a4<$,5,15,dd+dc>a8g8r8<cef8f+8   g>a<gec8>g8r<d+d8$,15,32,c4"
            ,
            "t4@0o3l16k8 f4g4a4b4<c8r2r8d+4"
            "o3$,5,15,k8 aga<k6c>a8g8r8k8a8$,15,32,a4$,5,15,k8aga<k6c>a8g8r8k8a8$,15,32,g4$,5,15,aga<k6c>a8g8r8k8a<cd8d+8   egec>a8k6g8r<c>a8$,15,32,g4"
            "o3$,5,15,k8 aga<k6c>a8g8r8k8a8$,15,32,a4$,5,15,k8aga<k6c>a8g8r8k8a8$,15,32,g4$,5,15,aga<k6c>a8g8r8k8a<cd8d+8   egec>a8g8r<c>a8$,15,32,g4"
            "o3$,5,15,k8 g+ag+k6ag+8e8r8a8$,15,32,a4$,5,15,k8g+ag+k6ag+8e8r8a8$,15,32,a4$,5,15, k8g+ag+k6ag+8k5e8r8a8$,15,32,a4$,5,15,aga<c>k6a8g+8g8<g8$,15,32,g4"
            "o3$,5,15,k8 aga<k6c>a8g8r8k5a8$,15,32,a4$,5,15,k8aga<k6c>a8g8r8k8a8$,15,32,g4$,5,15,aga<k6c>a8g8r8k8a<cd8d+8   egec>a8g8r<c>a8$,15,32,g4"
            "r1r2r8$,15,32,k8 @1r8o4d+4"
            "@0o3$,5,15,k8 aga<k6c>a8g8r8k8a8$,15,32,a4$,5,15,k8aga<c>a8g8r8k8a8$,15,32,g4$,5,15,aga<k6c>a8g8r8k8a<cd8d+8   egec>a8g8r<c>a8$,15,32,g4"
            ,
            "t4@2$,11,15,o1l8 a<a>b<bc<c>d<d>fr2r>b4"
            "o2 [3c.<c32r32>c<c>c<cc>c] g.g32r32>g<g>ggab"
            "o2 [3c.<c32r32>c<c>c<cc>c] g.<g32r32>g<g>c<cc>c"
            "o1 [3e.<e32r32>e<e>>a<a>a<a] g.<g32r32>g<g>ggab"
            "o2 [3c.<c32r32>c<c>c<cc>c] g.<g32r32>g<g>c<cc>c"
            "$,5,15,l16o1g8ggg8ggg8ggggggg8ggggggg8"
            "$,15,32,@1r4o4g8"
            "@2$,11,15,o2l8 [3c.<c32r32>c<c>c<cc>c] g.<g32r32>g<g>c<cc>c"
          );
          break;
        case '.':
          SetWaveForm (0, (const int8_t[]){48,80,80,48,0,0,16,64,96,112,96,48,-16,-32,-32,0,32,32,16,-64,-96,-112,-96,-64,0,0,-48,-80,-80,-48,0,0}, 32);
          ConvertAndStartMusic (5,
            "t7@0$,14,0,     o3l16v64 [2g4g+4r2]",
            "t7@0$,14,0, r32 o4l16v64 [2c4c+4r2]",
            "t7@0$,14,0, r16 o4l16v64 [2e4e+4r2]",
            "t7@0$,14,0, r16.o4l16v64 [2b-4b4r2]",
            "t7@0$,14,0, r8  o5l16v64 [2e-4e4r2]"
          );
          break;
#else
        // PSGによるドラム系の再現が野暮ったかったので全部PCMで代替
        // DAC更新周波数の都合で諸々の効果がイマイチなのとメモリと時間の都合で全て捨て
        case 'A':
          SetWaveForm (0, (const int8_t[]){0,64,127,64,1,-64,-127,-64,1,64,127,64,1,-64,1,64,1,-32,1,32,1,-16,1,16,1,-1,-1,-1,-1,64,64,64}, 32);
          SetWaveForm (1, (const int8_t[]){48,80,80,48,0,0,16,64,96,112,96,48,-16,-32,-32,0,32,32,16,-64,-96,-112,-96,-64,0,0,-48,-80,-80,-48,0,0}, 32);
          SetWaveForm (2, (const int8_t[]){0,-16,-32,-48,-64,-80,-96,-112,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,127,112,96,80,64,48,32,16}, 32);
          SetWaveForm (3, (const int8_t[]){0,48,80,96,112,96,80,48,0,-48,-80,-96,-112,-96,-80,-48,0,64,96,112,96,64,0,-64,-96,-112,-96,-64,0,112,0,-112}, 32);
          SetWaveForm (4, (const int8_t[]){70,-70}, 2);
          MSq[5].enable_PCM();  // set ch5 for PCM playback
          ConvertAndStartMusic (6,
            "t8 [@1$,10,33,o4l8k8v74 [2c+1c+c+c+c+rc+4. d1ddrdrd4.]"
            "r6 @2$,10,5,k10o4 [2v42e>a<c>a<e>a<ce4>v40a<v38c>v36a<v34av32ev30c>v28a<]"
            "o4 v42e>g<c>g<e>g<ce4>v40g<v38c>v36g<v34gv32ev30c>v28g< v35k9 ec+>a<aec+>a12< v50k8e4.d+&r2"
            "r2 r8 @1$,20,23, k8o3v40 a4.b&r1 a4a4a4.b&r1"
            "r2o4 d4.e&r2 >l16ef+gg+ab<cc+ l8d4d4d4.e&r1"
            "o3 r2a4.b&r1 a4a4a4.b&r1"
            "o4 r2d4.e2.&r8.&r32r32 $,20,3,K8v64 ec+>aec+ea <c+4>a4e4c+4>a]"
            ,
            "t8 [@3$,10,23,o3l8k8v50 [2a+1a+a+a+a+ra+4. b1bbrbrb4.]"
            "@2$,9,3,k10o4 [2v45e>a<c>a<e>a<c e4>v43a<v41c>v39a<v37av35ev32c>v29a<]"
            "o4 v45 e>g<c>g<e>g<ce4>v45g<v43c>v41g<v39gv37ev35c>v32g<  k8v50 ec+>a<aec+>a<c+ c+4.c+&r2"
            "$,25,50,o3v45 [3r2 a4.b8&r1 a4a4a4.b8&r1] r2a4.b8&r2.&r8.&r32r32"
            "@3$,9,4,o4v50 c+c+c+c+c+c+rc+rc+rc+rc+c+c+]"
            ,
            "t8 [@3$,10,23,o3l8v50 [2f+1f+f+f+f+rf+4. g1ggrgrg4.]"
            "@2$,9,3,o4 [2v50e>a<c>a<e>a<c e4>v47a<v44c>v41a<v38av35ev32c>v29a<]"
            "o4 v50 e>g<c>g<e>g<ce4>v47g<v44c>v41g<v38gv35ev32c>v29g<  v50 c+>ae<ec+>aea a4.a&r2"
            "$,25,50,o3v45 [3r2 f+4.g8&r1 f+4f+4f+4.g8&r1] r2f+4.g8&r2.&r8.&r32r32"
            "@3$,9,4,o3v50 aaaaaararararaaa]"
            ,
            "t8[@0k10v25$,25,33,o1l8 [4 er<er>ee<e>ere<e>eee<e>e] fr<fr>ff<f>frf<f>fff<f>f f+r<f+r>f+f+<f+>f+rf+<f+>f+f+f+<f+>f+ gr<gr>gg<g>grg<g>ggg<g>g f+r<f+r>f+f+<f+>f+rbl16bbl8bf+>bbb"
            "@0k10$,25,33,v25o1 eb<e>e16.&r64r64 @1k9v64$,20,23, o3 a4.b&r1 a4a4a4.b8&r1"
            "@0k10$,25,33,v25o2 c>ge<c16.&r64r64 @1k9v64$,20,23, o4 d4.e&r2 >l16ef+gg+ab<cc+ l8d4d4d4.e&r1"
            "@0k10$,25,33,v25o1 eb<e>e16.&r64r64 @1k9v64$,20,23, o3 a4.b&r1 a4a4a4.b&r1"
            "@0k10$,25,33,v25o2 c>ge<c16.&r64r64 @1k9v64$,20,23, o4 d4.e&r1 $,20,3, aec+>aea<c+ e4c+4>a4e4c+]"
            ,
            "t8 @4$,10,0,l8V70 [o1 [4e4<e4>ee<e>e4e<e>eee<e>e]"
            "f4<f4>ff<f>f4f<f>fff<f>f"
            "f+4<f+4>f+f+<f+>f+4f+<f+>f+f+f+<f+>f+"
            "g4<g4>gg<g>g4g<g>ggg<g>g"
            "o1 f+4<f+4>f+f+<f+>f+4bl16bbl8bf+>bbb"
            "eb<e>e[3deee]eb<e>e[2deee]degb"
            "o2 c>ge<c [6>b<ccc] >b<c>gd"
            "o1 eb<e>e[3deee]eb<e>e[2deee]degb"
            "o2 c>ge<c [2>b<ccc] >b<cc>g $,12,45,f+1$,10,0, b2abbb]"
            ,
            "t8l8o4 [v64[8@0c@2c24c24c24@1c@2c@0cc@1c@2c @0c@2c@1c@2c@0cc@1c@3c]"
            "[3[3@1c@0c@2c@0c]@1cc@2c@0c] [3@1c@0c@2c@0c] v30@4>g<v64@1ccc"
            "[3[3@1c@0c@2c@0c]@1cc@2c@0c] @3c2v30@4>g<v64@2c16c16@1cc@3c2 @1cccc]"
          );
          break;
        case 'S':
          SetWaveForm (0, (const int8_t[]){-70,70}, 2);
          SetWaveForm (1, (const int8_t[]){48,80,80,48,0,0,16,64,96,112,96,48,-16,-32,-32,0,32,32,16,-64,-96,-112,-96,-64,0,0,-48,-80,-80,-48,0,0}, 32);
          SetWaveForm (2, (const int8_t[]){-8,-16,-24,-32,-40,-48,-56,-64,-72,-80,-88,-96,-104,-112,-120,-128,120,112,104,96,88,80,72,64,56,48,40,32,24,16,-8,0}, 32);
          SetWaveForm (3, (const int8_t[]){0,-16,-32,-48,-64,-80,-96,-112,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,127,112,96,80,64,48,32,16}, 32);
          SetWaveForm (4, (const int8_t[]){-96,-112,-112,-112,-96,-96,-80,-80,-64,-64,-48,-48,-32,-32,-16,-16,0,0,16,16,32,32,48,48,64,64,80,80,96,96,96,80}, 32);
          MSq[5].enable_PCM();  // set ch5 for PCM playback
          ConvertAndStartMusic (6,
            "t6[l4r @1v56$,32,,o3 g.a.g.a$,10,,g8a8b8 r$,32,,g.a.g.a$,10,,<e8d8c+8>"
            "r @1v56$,32,, g.a.g.a$,10,,g8a8b8 r$,32,,g.a.g.a<@2$,10,,e8d8c+8"
            "@1$,10,,l16 cc8>aa8bb8gg8aa8f+f+8r4 $,32,,g4.f+4.<"
            "$,10,,cc8>aa8bb8gg8aa8f+f+8<e8d8c+8>$,32,,b4. @3v61$,15,, b<c+de"
            "v65f+4.e4d8c8d8e4.g4<c8e4>>"
            "@2v52$,7,, gg8e8ee8 aa8f+8f+f+8 bb8g8ga8<"
            "dd8c+8>bag bb8 g8g e8 aa8 f+8f+a8 agf+ef+8ef+8 bb $,7,,v65b8bb8]"
            ,
            "t6[r6l4r @1k10v26$,32,,o3 g.a.g.a @2v17$,8,,g8a8b8$,32,, r @1v26g.a.g.a@2$,8,, v17<e8d8c+8>$,32,,"
            "r @1v27g.a.g.a @2v17$,8,,g8a8b8$,32,, r @1v26g.a.g.a$,8,, @2v17<e8d12$,32,,"
            "@1v45$,10,,l16 c8.cc8>a8.bb8g8.aa8 @2$,32,,v39k8 b8&l32[6b&k10b&k8b&k6b&]b&k10b&k8b&k6bl16k10<"
            "@1v45$,10,, c8.cc8>a8.bb8g8.aa8 @2$,32,,v39@4k8 b8&l32[6b&k10b&k8b&k6b&]b&k10b&k8b&k6bl16k10<"
            "@2v17$,10,, f+6f+4.e4d8c8d8e4.g4<c8e4>>"
            "v17 gg8e8ee8 aa8f+8f+f+8 bb8g8ga8<"
            "dd8c+8>bag bb8 g8g e8 aa8 f+8f+a8 agf+ef+12"
            "v65k8l64 e&f&f&f+&g&g+&g+&a&a&a+&a+&bl16 $,7,,e8ee8]"
            ,
            "t6[[2l4r @1v56$,32,,o3 b.<c+.>b.<c+ @2$,8,,v61>b8<c+8d8>"
            "r @1v56$,32,, b.<c+.>b.<c+ @2$,8,,v61>g8f+8e8]"
            "@1v43$,32,,o4l16 ee8c8.dd8>b8.<c+c+8>a8.r4b4r8a4.<"
            "ee8c8.dd8>b8.<c+c+8>a8.g8f+8g8e4."
            "@3v61$,15,, def+g v65a4.g4f+8e8f+8g4.<c4e8g4>"
            "v52$,7,, ee8>b8bb8<f+f+8d8dd8 gg8e8ef+8 bb8a8gf+e"
            "gg8e8eb8 f+f+8dd8f+8 ed>b<d8>b<d8>"
            "v65$,32,, l64 b&<c&c&c+&d&d+&d+&e&f&f+&f+&gl16 $,7,,g8gg8]"
            ,
            "t6[@1v52o2 [2$,32,,l32ef+e8.&r2.r1 ef+e8.&r2.< $,8,,e8.e16d8e8$,32,,r2 >]<<"
            "v52l16$,32,, ec>afa<c d>bgegb <c+>af+df+a g1 ecafa<c d>bgegb <c+>af+df+a e1>"
            "@4 e16d8.&r2. c16&r8.&r2.<"
            "@3$,15,,v52@k6 ede4.geg4.bgb4.geg4."
            "bgb4.&r2 <v65 d2 l16$,7,,eeee8ee8]"
            ,
            "t6v80 @0$,4,0,l16 [o1[4 ee>e<edeed dee8dee8 ee>e<edeed ee8deegb]"
            "fff>ece< eee>d<bg dddf+da ee>e<e deee e8>e<e deeb"
            "fff>ece< egb>d<be dddf+da ee>e<e deee e8>e<e cdef+"
            "dd>d<dcddc cdd8cdd8 cc>c<ccccc ce8eggga"
            "ee>e<edeed dee8dee8 ee>e<edeed dee8eegb"
            "ee>e<edeed dee8dee8 ee>e<edeed de8decg8]"
            ,
            "t6l16v64 [[2 @0c8@2cc@1c8@2cc @0c8@2cc@1c8@2c@0c c8@2cc@1c8@2cc @0c8@2cc@1c8cc"
            "@0c8@2cc@1c8@2cc @0c8@2cc@1c8@2c@0c c8@2cc@1c8@2cc @0c8@2cc@1cc@2c@1c ]"
            "[2 @0c@2ccc@1cc @0c@2ccc@1cc @2ccc@0c@1c@3c @0c8@2cc@1c8@2cc @0c8@2cc @1cc@3c@1c ]"
            "[2 @0c8@2cc@1c8@2cc @0c8@2cc@1c8@2c@0c c8@2cc@1c8@2cc @0c8@2cc@1c8cc ]"
            "@0c8@2cc@1c8@2cc @0c8@2cc@1c8@2c@0c c8@2cc@1c8@2cc @1cccc8cc8]"
          );
          break;
        case 'D':
          SetWaveForm (0, (const int8_t[]){0,25,49,71,90,106,117,125,127,125,117,106,90,71,49,25,-128,-112,-96,-80,-64,-48,-32,-16,0,16,32,48,64,80,96,112}, 32);
          SetWaveForm (1, (const int8_t[]){0,64,127,64,1,-64,-127,-64,1,64,127,64,1,-64,1,64,1,-32,1,32,1,-16,1,16,1,-1,-1,-1,-1,64,64,64}, 32);
          SetWaveForm (2, (const int8_t[]){0,64,127,64,0,-64,-1,-64,5,-21,-42,-61,-71,-81,-92,-100,-107,-113,-119,-124,-127,-124,-119,-113,-107,-100,-92,-81,-71,-61,-42,-22}, 32);
          SetWaveForm (3, (const int8_t[]){0,78,98,109,117,122,125,126,127,126,125,122,117,109,98,78,0,-79,-99,-110,-118,-123,-126,-127,-128,-127,-126,-123,-118,-110,-99,-79}, 32);
          SetWaveForm (4, (const int8_t[]){0,64,127,64,1,-64,-127,-64,1,64,127,64,1,-64,1,64,1,-32,1,32,1,-16,1,16,1,-1,-1,-1,-1,64,64,64}, 32);
          SetWaveForm (5, (const int8_t[]){0,-16,-32,-48,-64,-80,-96,-112,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,-128,127,112,96,80,64,48,32,16}, 32);
          SetWaveForm (6, (const int8_t[]){0,48,80,96,112,96,80,48,0,-48,-80,-96,-112,-96,-80,-48,0,64,96,112,96,64,0,-64,-96,-112,-96,-64,0,112,0,-112}, 32);
          SetWaveForm (7, (const int8_t[]){48,80,80,48,0,0,16,64,96,112,96,48,-16,-32,-32,0,32,32,16,-64,-96,-112,-96,-64,0,0,-48,-80,-80,-48,0,0}, 32);
          SetWaveForm (8, (const int8_t[]){-70,70}, 2);
          MSq[5].enable_PCM();  // set ch5 for PCM playback
          ConvertAndStartMusic (6,
            "t5 [@0$,64,70,v64o3l12 e-6e-b-2&rb-<cd-2.d-cd-c6>a-e-2. @1v42 e-6e-r4e-6e-r4"
            "@0v64 e-6e-b-2&ra-g-<d-2g-6d-6>b-6a-4&ra-b-<c2 @2v50 e-4&rfe-d2>"
            "@0v64 e-6e-b-2&rb-<cd-2.d-cd-c6>a-e-2.< @2v50 e-4&rfe-d6e-6f6>"
            "@0v64 e-6e-b-2&ra-g-<d-2g-6d-6>b-6a-4&ra-b-<c2 @3$,10,0,v34 de-fe-fg-fg-a-b-<d-e->>>"
            "@4$,5,10,v48 g-a-<ce-g-a-< @5$,64,70, e-4&rc>a-b-4&rb-<cd-e-fe-4>>"
            "@4$,5,10, a-b-<dfa-b-< @5$,64,70, f4&rd>b-<c4&rcdde-fe-4"
            "g-a-b-e-4e-fg-a-g-fe-e-e-b-8&l32[5k8b-&k11b-&k8b-&k5b-&]k8l12"
            "v38 b-2<c2>>d-4.c4>b4.<"
            "@0v45 b-6d-6g-6b-<cd-e-d-c> b-6e-6a-6b-<cd-e-d-c> b-6b-<e-2>b-<e-a-g1]"
            ,
            "t5 [@0$,64,70,v35o3k10l12 e-16e-6e-b-2&rb-<cd-2.d-cd-c6>a-e-2&r8. @1v42k8 b-6b-r4b-6b-r4"
            "@0v35k10 e-16e-6e-b-2&ra-g-<d-2g-6d-6>b-6a-4&ra-b-<c2 @3 e-4&rfe-d2>"
            "@0v35 e-6e-b-2&rb-<cd-2.d-cd-c6>a-e-2&r8. @2v40K8 b-4&r<d>b-b-6<c6d6>"
            "@0v35k10 e-16e-6e-b-2&ra-g-<d-2g-6d-6>b-6a-4&ra-b-<c2 @3$,10,0,v25 de-fe-fg-fg-a-b-<d-e-48>>>"
            "@4$,5,10,v48 g-a-<ce-g-a- @5$,64,70, e-4&rc>a-b-4&rb-<cd-e-fe-4>"
            "@4$,5,10, a-b-<dfa-b- @5$,64,70, f4&rd>b-<c4&rcdde-fe-4<"
            "v29k10 g-16g-a-b-e-4e-fg-a-g-fe-e-e-l32[5k10b-&k13b-&k10b-&k7b-&]k10b-16l12"
            "v38k8 e-2f2>g-4.f4e4."
            "@6v45k10 b-6d-6g-6b-<cd-e-d-c> b-6e-6a-6b-<cd-e-d-c> b-6b-j8b-2k10b-<e-a-k8e-1]"
            ,
            "t5 [@7$,5,0,v38o3l12 [8[6a-b-<e->b-]]"
            "v40$,40,64, b-2<c2d-2c4>b-4<c2d2e-2d4c4>"
            "[2v40$,40,64, g-2a-2< v38$,5,0, [2d->b-g-<]c>afbg+e>]"
            "v51$,40,64, g-1< @6v34a-1"
            "@7v38$,5,0, [6a-b-<e->b-]]"
            ,
            "t5 r8[@7k10$,5,0,v28o3l12 [7[6a-b-<e->b-]] [5a-b-<e->b-] a-b-<e-24 >"
            "v40$,40,64,k8 g-2a-2a-2a-4g-4a-2b-2<c2>b-4a-4"
            "[2v40 k8$,40,64, d-2e-2< v28k10$,5,0, d-8[2d->b-g-<]c>afbg+24]"
            "v51$,40,64,k8 d-1< @6v34e-1>"
            "@7k10$,5,0,v28 a-8 [6a-b-<e->b-]]"
            ,
            "t5 [@8$,7,0,v60o1l12 [4[2e-e-e-<e->e-e-][2g-g-g-<g->g-g-][2a-a-a-<a->a-a-][2b-b-b-<b->b-b-]]"
            "[2g-g-a-<a->a-a-[3g-a-a-<a->a-a-]]"
            "[4d-de-<e->e-<e->d-de-b-<e->b-]"
            "e-ef[3g-g-g-] ga-a-[3a-a-a-] [3e-e-e-]e-a-b- <[2e-e-e-] e-6>b-6g6]"
            ,
            "t5l12v48 [ [[6[3@0c@2c24@2c24@0c@1c@0c@2c @0c@2c@0c@1c@2c@3c] @0c@2c24@2c24@0c@1c@1c@2c @0c@2c24@2c24@0c@1c@1c@1c]"
            "@0c@2c24@2c24@2c@1c@2c@0c @0c@2c@0c@1c@2c@0c [3@1c@0c@0c] @1c24@1c24@1c@1c"
            "@0c@2c24@2c24@0c@1c@2c@0c @0c@2c24@2c24@0c@1c@2c@2c @0c@2c24@2c24@0c@1c@2c@0c @0c@1c24@1c24@1c@1c@1c@1c ]"
          );
          break;
        case 'F':
          SetWaveForm (0, (const int8_t[]){-70,70}, 2);
          SetWaveForm (1, (const int8_t[]){0,0,0,-128,0,112,112,112,0,0,0,-128,0,0,0,-128,-128,-128,-128,0,-128,0,0,0,0,-128,-128,-128,0,-128,-128,-128}, 32);
          SetWaveForm (2, (const int8_t[]){0,-8,-16,-24,-32,-40,-48,-56,-64,-72,-80,-88,-96,-104,-112,-120,-128,120,112,104,96,88,80,72,64,56,48,40,32,24,16,-8}, 32);
          SetWaveForm (3, (const int8_t[]){48,80,80,48,0,0,16,64,96,112,96,48,-16,-32,-32,0,32,32,16,-64,-96,-112,-96,-64,0,0,-48,-80,-80,-48,0,0}, 32);
          MSq[5].enable_PCM();  // set ch5 for PCM playback
          ConvertAndStartMusic (6,
            "t5@1$,5,0,l16 [o1v56 [7c8<c>c8c<c>c]c8<c>c8cb->b-< [7c8<c>c8c<c>c]c8<c>c8c<d>d"
            "v60 [4e-8<e->e-8e-<e->e-] [2d8<d>d8d<d>d] [2g8<g>g8g<g>g] [4e-8<e->e-8e-<e->e-] [2d8<d>d8d<d>d] g8<g>g8g<g>g fggf gggg]"
            ,
            "t5$,64,100,1 [@2o3v40l8 c2d2f2b-.f.b-g.l16cr2<fedcl8d.f.b-4agf>"
            "c2d2f2b-.f.b-g.<l16cr2crfe&r2.&r8r8"
            "@3o3v40l8 b-32<e-32ge-.>b-4<ge->b- <c32f32af.c4afc"
            "c32f32af.c4afc l16br<cd8.>>br<cd&r4.>"
            "v55l8 b-32<e-32ge-.>b-4<b-ge- c32f32af.c4<c>af"
            "c32f32af.c4<fc>a <cc16>b.g.d.c>b]"
            ,
            "t5$,64,100,1 [@2o3v40l8 c2c2c2d.>b-.<.dc.l16>gr2<<dc>b-al8b-.<d.f4dc>b-"
            "c2c2c2d.>b-.<.dc.l16gr2gr<cc&r2.&r8r8"
            "@3o4v40l8 e-.>b-.g4<e->b-g <f.c.>a4<c>af"
            "<f.c.>a4<c>af< l16grab8.>grab&r4."
            "v55l8 e-32g32b-g.e-4<e->b-g f32a32<c>a.f4<fc>a"
            "r16<fc.>f4<afc ff16g.d.>b.gd]"
            ,
            "t5$,64,100,1 [@2o3v30k10l8 c2d2f2b-.f.b-g.l16cr2<fedcl8d.f.b-4agf>"
            "c2d2f2b-.f.b-g.l16cr2<crfe&r2.&r8r8"
            "@3o3v30k9 b-32<e-32l8ge-.>b-4<ge->b- <c32f32af.c4afc"
            "c32f32af.c4afc l16br<cd8.>>br<cd&r4.>"
            "b-32<e-32l8ge-.>b-4<b-ge- c32f32af.c4<c>af"
            "c32f32af.c4<fc>a <cc16>b.g.d.<k9l32v50edc>bagfe]"
            ,
            "t5@0v60$,2,30,5 [l16o5 [7c>cc<c >cc<c>c<] c>cc<c >ccb->b-<<"
            "[7c>cc<c >cc<c>c<] c>cc<c >cc<d>d<"
            "[4e->e-e-<e- >e-e-<e->e-<] [2d>dd<d>dd<d>d<] [2g>gg<g>gg<g>g<]"
            "[4e->e-e-<e- >e-e-<e->e-<] [2d>dd<d>dd<d>d<] >>gab<cdefgab<cdl32edc>bagfe<]"
            ,
            "t5l16v64 [[8@0cr@1c@0c rc@1c@0c][8@0cr@1c@0c rc@1c@0c]"
            "[8@0cr@3c@0c r@0c@3c@0c][7@0cr@3c@0c r@0c@3c@0c] @0c@1cc@0c @1ccc@0c]"
          );
          break;
        case 'G':
          SetWaveForm (0, (const int8_t[]){0,-8,-16,-24,-32,-40,-48,-56,-64,-72,-80,-88,-96,-104,-112,-120,-128,120,112,104,96,88,80,72,64,56,48,40,32,24,16,-8}, 32);
          SetWaveForm (1, (const int8_t[]){0,25,49,71,90,106,117,125,127,125,117,106,90,71,49,25,-128,-96,-64,-32,0,32,64,96,-128,-96,-64,-32,0,32,64,96}, 32);
          SetWaveForm (2, (const int8_t[]){112,112,112,112,112,112,112,112,-128,-128,-128,-128,-128,-128,-128,-128,112,112,112,-128,-128,-128,112,112,112,112,-128,-128,-128,-128,-128,-128}, 32);
          SetWaveForm (3, (const int8_t[]){0,0,0,-128,0,112,112,112,0,0,0,-128,0,0,0,-128,-128,-128,-128,0,-128,0,0,0,0,-128,-128,-128,0,-128,-128,-128}, 32);
          SetWaveForm (4, (const int8_t[]){0,25,49,71,90,106,117,125,127,125,117,106,90,71,49,25,-128,-112,-96,-80,-64,-48,-32,-16,0,16,32,48,64,80,96,112}, 32);
          SetWaveForm (5, (const int8_t[]){80,-80},2);
          MSq[5].enable_PCM();  // set ch5 for PCM playback
          ConvertAndStartMusic (6,
            "t5@0o3l16v64 [2gg<[2ec>g<f+d>g]ec] [2aa<[2f+d>a<gd>a]f+d]"
            "[@1v56 [2gg<[2ec>g<f+d>g]ec] [2aa<[2f+d>a<gd>a]f+d]"
            "<[2[2ecef+df+]eceg] [2[2f+df+geg]f+df+a]]"
            ,
            "t5@2o1l16v64 [2c4.&r16r8.cr8crc] [2d4.&r16r8.dr8drd]"
            "[[2l16v48ccrggr<ccrggr<c8l32v36gec>g]>"
            "[2l16v48ddraar<ddraar<d8l32v36af+d>a]"
            "[2l16v48ccrggr<ccrggr<c8l32v36gec>g]>"
            "[2l16v48ddraar<ddraar<d8l32v36af+d>a]>>]"
            ,
            "t5@3o2l16v56$,3,,5 [2gg<[2ec>g<f+d>g]ec] [2aa<[2f+d>a<gd>a]f+d]<"
            "[@4v64 [2gg<[2ec>g<f+d>g]ec] [2aa<[2f+d>a<gd>a]f+d]>>"
            "@3[2l16v48 ggr<ccrggr<ccrg8l32v36<c>gec]>>"
            "[2l16v48 aar<ddraar<ddra8l32v36<d>af+d]>>]"
            ,
            "t5k7@3o2l16v56$,3,,5 [2g4.&r16r8.gr8grg] [2a4.&r16r8.ar8ara]>"
            "[@4k7 [2l16v48 ggr<ccrggr<ccrg8l32v36<c>gec]>>"
            "[2l16v48 aar<ddraar<ddra8l32v36<d>af+d]<"
            "@3v48l16 [2[2ecef+df+]eceg] [2[2f+df+geg]f+df+a]]"
            ,
            "t5@5o3l16v50 [2e2.r8fg] [2d2.r8dd]"
            "[v64 [2[4ccr][2cr]] [2[4ddr][2dr]]<"
            "v56 [8c<c>][8c<c>] [8d<d>][8d<d>]]"
            ,
            "t5v64o4 [4l4@2c@1ccc8l16@2cc] @1[[[4ccr][2cr]]8]"
          );
          break;
        case 'H':
          SetWaveForm (0, (const int8_t[]){0,-8,-16,-24,-32,-40,-48,-56,-64,-72,-80,-88,-96,-104,-112,-120,-128,120,112,104,96,88,80,72,64,56,48,40,32,24,16,-8}, 32);
          SetWaveForm (1, (const int8_t[]){0,25,49,71,90,106,117,125,127,125,117,106,90,71,49,25,-128,-96,-64,-32,0,32,64,96,-128,-96,-64,-32,0,32,64,96}, 32);
          SetWaveForm (2, (const int8_t[]){80,-80}, 2);
          MSq[5].enable_PCM();  // set ch5 for PCM playback
          ConvertAndStartMusic (6,
            "t5@0o3l16v58$,50,100, [l2 g1&r2<c>a1&r2<dc1&r2gf+1&r2 l32a&a&a-&g&g-&f&e&e-&d&d-&c&>b&b-&a&a-&g]"
            ,
            "t5@0o3l16v36$,50,100,k10 r4 [l2 g1&r2<c>a1&r2<dc1&r2gf+1&r2 l32a&a&a-&g&g-&f&e&e-&d&d-&c&>b&b-&a&a-&g]"
            ,
            "t5@0o3l16v48$,8,10,K10 [[2gg<ec>g<f+d>g<ec>g<f+d>g<ec>] [2aa<f+d>a<gd>a<f+d>a<gd>a<f+d>]]"
            ,
            "t5@1o2l16v48$,8,10, [[2gg<ec>g<f+d>g<ec>g<f+d>g<ec>] [2aa<f+d>a<gd>a<f+d>a<gd>a<f+d>]]"
            ,
            "t5@2o2l16v40 [[2c2r8c.r.crc][2d2r8d.r.drd]]"
            ,
            "t5v50o4l16 [[7@0c@2c@1c@2c @0c@0c@1c@2c] [8@1c]]"
          );
          break;
#endif
        // 再生停止
        case ' ':
          StopMusic ();
          break;

        // 最終段ベロシティーの変更
        case '(':
          PCM.set_volume (PCM.get_volume () + 1);
          dx.printf ("\r%3d", PCM.get_volume ());
          break;
        case ')':
          PCM.set_volume (PCM.get_volume () - 1);
          dx.printf ("\r%3d", PCM.get_volume ());
          break;

        case '!':
          UD5_SOFTRESET();
          break;
      } //swith
    } //while

    UD5_LOWPOWERMODE(); // wait for Interrupt
  } //for
}


//! main関数
int main (void) {
  // タスク作成
  xTaskCreate (KeyTask, NULL, 150, NULL, 1, NULL);
  // PCMを紐付け
  MSq[0].begin(&PCM);

  // OSの動作開始
  vTaskStartScheduler ();
}
