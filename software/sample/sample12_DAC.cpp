/*!
 @file  sample12_DAC.cpp
 @brief PCMデータ再生
 @note
  取り込んだRAWファイルを元にGPIO7からアナログ波形を出力。
  スピーカーアンプをつなげば音となる。
 */
#include <ud5.h>

CDXIF dx;

//! RAWファイルの埋め込み (11.025kHz Signed 8bit RAW C4前提)\n
//! バイナリファイルの埋め込み方詳細は<a href="https://gist.github.com/mmozeiko/ed9655cf50341553d282">こちら</a>\n
//! wavは<a href="https://freewavesamples.com">こちら</a>から拝借\n
//! raw変換は<a href="http://sox.sourceforge.net/">sox</a>
INCBIN ("5.raw", pcm_beep);
//! RAWファイルの埋め込み
INCBIN ("6.raw", pcm_hit);
//! RAWファイルの埋め込み
INCBIN ("7.raw", pcm_btn);

//! PCMの情報を一元化(const化禁止)
std::vector<CPCM::TPCMInfo> PCMInfo{
  { (int8_t *) &pcm_beep, (int) &_size_pcm_beep, -1, 256, 127 },
  { (int8_t *) &pcm_hit, (int) &_size_pcm_hit, -1, 256, 127 },
  { (int8_t *) &pcm_btn, (int) &_size_pcm_btn, -1, 256, 127 },
};

//! GPIO7をDAC出力に構成
CPCM pcm (&PCMInfo, 32);

//! main関数
int main (void) {
  while (1) {
    if (dx.rxbuff()) {
      char c = dx.getc();
      dx.putc (c);
      switch (c) {
        case '@':
          pcm.play (0, 48, 64);
          break;

        case 'a' ... 'j':
          pcm.play (1, 48 + c - 'a', 64);
          break;
        case '0' ... '9':
          pcm.play (2, 60 + c - '0', 64);
          break;
        case '!':
          return 0;

        case '<':
          dx.printf ("\nvol=%d\n", pcm.set_volume (pcm.get_volume() + 1));
          break;
        case '>':
          dx.printf ("\nvol=%d\n", pcm.set_volume (pcm.get_volume() - 1));
          break;

        case 'B':
          pcm.begin();
          break;
        case 'E':
          pcm.end();
          break;

        default:
          break;
      }
    }
    UD5_WAIT (50);
  }
}
