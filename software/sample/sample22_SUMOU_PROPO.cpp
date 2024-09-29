/*---------------------------------------------
  ラジコンプロポ用サンプル
  ---------------------------------------------*/
/*
  プロポの2つのスティックを使ってモータを運転。
  プロポの信号はCN7-1とCN7-2に入力。
  モータのイネーブル・ディスエーブルはGPIO9の押下で切り替え。
  必要に応じて土俵センサをJP4とJP5に接続。
  特段必要ではないがRTOSを使って3つのタスクに処理を分担させた。

  TASK1:
  JP16によるモータの大本のイネーブル処理。
  プロポの調整はターミナル上で'c'を入力。
  その後の操作はターミナル上のメッセージを見ながらキー入力。

  TASK2:
  センサの取り込みはSPI経由で時間がかかるため、ここで一括取り込み。
  受信機のパルス幅を測定し±1000‰へ変換。
  パルスに変化量を持たせた上でモータへ指令。
  周期を1msとしているので重量級の処理は禁物。

  TASK3:
  センサに反応があったらプロポの信号を無視しセンサの反応が消えるまでバック。
  誤った反応等でバックし過ぎないようにタイムアウトあり。
*/

#include <ud5.h>

CDXIF dx;

// MCUのGPIOを初期化
CGPIO gpio (
  (const CGPIO::TPinMode[10]) {
    CGPIO::tPinINT0_MPW,  // 右モータ用受信機パルス
    CGPIO::tPinINT1_MPW,  // 左モータ用受信機パルス
    CGPIO::tPinINT2_MPW,  // その他パルス取り込み1
    CGPIO::tPinINT3_MPW,  // その他パルス取り込み2
    CGPIO::tPinDIN_PU,    // DIP0
    CGPIO::tPinDIN_PU,    // DIP1
    CGPIO::tPinDIN_PU,    // DIP2
    CGPIO::tPinSPFUNC,    // DAC
    CGPIO::tPinDIN_PU,    // DIP3
    CGPIO::tPinDIN_PU,    // PB1
  }
);

// プロポ取り込みとgpioを紐付け
CRCStick rcstick (&gpio);

// 拡張I/Oを初期化
CEXIO exio (
  (const CEXIO::TPinMode[16]) {
    CEXIO::tPinDIN_PU,  // 右側物体検出センサ
    CEXIO::tPinDIN_PU,  // 中央物体検出センサ
    CEXIO::tPinDIN_PU,  // 左側物体検出センサ
    CEXIO::tPinDIN_PU,  // 右側土俵検出センサ
    CEXIO::tPinDIN_PU,  // 左側土俵検出センサ
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU
  }
);

// GPIOとJPのビットアサイン
union bf_gpio {
  struct {
    uint16_t      : 1;
    uint16_t      : 1;
    uint16_t      : 1;
    uint16_t      : 1;
    uint16_t dip0 : 1;
    uint16_t dip1 : 1;
    uint16_t dip2 : 1;
    uint16_t dac  : 1;
    uint16_t dip3 : 1;
    uint16_t pb1  : 1;
  };
  uint16_t u16;
} mmi;

uint8_t dip;

union bf_exio {
  struct {
    uint16_t TR : 1;
    uint16_t TC : 1;
    uint16_t TL : 1;
    uint16_t RR : 1;
    uint16_t RL : 1;
  };
  uint16_t u16;
} sen;

// RAWデータファイルを直接取り込み
INCBIN ("5.raw", pcm_beep);
INCBIN ("6.raw", pcm_hit);
INCBIN ("7.raw", pcm_btn);

// PCMの情報を一元化(const化禁止)
std::vector<CPCM::TPCMInfo> PCMInfo{
  { (int8_t *) &pcm_beep, (int) &_size_pcm_beep, -1, 256, 127 },
  { (int8_t *) &pcm_hit, (int) &_size_pcm_hit, -1, 256, 127 },
  { (int8_t *) &pcm_btn, (int) &_size_pcm_btn, -1, 256, 127 },
};

// GPIO7をDAC出力に構成
CPCM pcm (&PCMInfo, 64);

// モータへのランプ指令増分(0～1000‰/ms)
#define _MOTOR_RAMP       (50)
// スティックの中立位置の不感帯[%]
#define _NEUTRAL_POS_DEAD (3)
// スティックの最大位置の不感帯[%]
#define _MAX_POS_DEAD     (5)
// 最大出力値(ブートストラップ回路への充電時間を加味する事)
#define _MAX_MOTOR_VALUE  (1000-20)

// モータを初期化
CMotor motor (
  50000,  // PWM周波数
  0,      // モータの回転方向
  (uint16_t[2]) { _MAX_MOTOR_VALUE, _MAX_MOTOR_VALUE } // 最大PWM値(‰)
);

// モータ用プロポのパルス幅の測定値(モニタ用)
int32_t m1_pulse, m2_pulse;

// プロポ操作の許可
bool active = true;

uint8_t md = 0;

//---------------------------------------------------------------------------------
// キャリブレーション関数用コールバック
//---------------------------------------------------------------------------------
// 次の状態に遷移させるトリガ
bool next_step (void) {
  // UARTから何かしら受信したら真を返す
  if (dx.rxbuff()) {
    dx.clear_rxbuff();
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------------
// ターミナルによる雑処理タスク
//---------------------------------------------------------------------------------
void TASK1 (void *pvParameters) {
  UD5_WAIT (100);
  dx.puts ("\n>START\n");

  for (;;) {
    // ゲートのON/OFF
    if (!mmi.pb1) {
      exio.set_LED_off (0xf);
      motor.set_gate (!motor.get_gate());
      if (motor.get_gate()) {
        dx.puts ("\n>GATE ON\n");
        pcm.play(0, 48, 127);
        UD5_WAIT(100);
        pcm.play(0, 60, 127);
        UD5_WAIT(100);
      } else {
        dx.puts ("\n>GATE OFF\n");
        pcm.play(0, 60, 127);
        UD5_WAIT(100);
        pcm.play(0, 48, 127);
        UD5_WAIT(100);
      }
      while (!mmi.pb1) UD5_WAIT (20);
      UD5_WAIT (20);
    }

    // ゲートON/OFFの状態をLEDに反映
    if (motor.get_gate()) exio.set_LED_toggle (0xf);  // LED点滅
    else                  exio.set_LED_off (0xf);     // LED消灯

    // 現在の諸々の状態を逐次表示
    dx.printf ("stick: [0]%7ld->%5ld  [1]%7ld->%5ld  dir:%d  md:%d dip:0x%1X sen:0x%04X\r", m1_pulse, motor.get_duty(0), m2_pulse, motor.get_duty(1), motor.get_swap_dir(), md, dip, sen.u16);

    // 半二重であるために受信しにくいがUARTからのコマンド受付
    if (dx.rxbuff()) {
      switch (dx.getc()) {
        // 終了
        case '!':
          UD5_SOFTRESET();
          break;
        // プロポの調整
        case 'c':
          // モータ駆動停止
          motor.set_gate (false);
          // キャリブレーション開始
          rcstick.calibration (next_step);
          break;
      }
    }
    UD5_WAIT (100);
  }
}

//---------------------------------------------------------------------------------
// センサ取り込み・受信機のパルス取り込み・モータへのランプ指令
//---------------------------------------------------------------------------------
void TASK2 (void *pvParameters) {
  int32_t m1 = 0, m2 = 0;
  portTickType t = xTaskGetTickCount();
  uint32_t pulse[4], prev_pulse[4];
  uint32_t nochange_cnt[4] = {0,0,0,0};

  for (;;) {
    mmi.u16 = gpio.get_gpio();
    // DIPスイッチの取り込み
    dip = ~(mmi.dip0 | (mmi.dip1 << 1) | (mmi.dip2 << 2) | (mmi.dip3 << 3)) & 0xf;
    // センサ取り込み
    sen.u16 = ~exio.get_gpio();
    // プロポの取り込みとモータへのランプ指令
      // パルス幅測定値から不感帯を加味した‰値へ変換
    m1_pulse = rcstick.get_normal (0, _NEUTRAL_POS_DEAD, _MAX_POS_DEAD);
    m2_pulse = rcstick.get_normal (1, _NEUTRAL_POS_DEAD, _MAX_POS_DEAD);

    for (int i = 0; i < 4; i++) {
      pulse[i] = gpio.get_pwd(i);
      if (pulse[i] == prev_pulse[i]) nochange_cnt[i]++;
      else nochange_cnt[i] = 0;
      prev_pulse[i] = pulse[i];
    }

    gpio.get_pwd(2);
    uint32_t p = gpio.get_pwd(3);
    uint32_t pw[] = {27500,39000,45900,49500};
    for (int i = 0; i < 4; i++)
      if ((pw[i] - 300) < p  &&  p < (pw[i] + 300)) md  = i;


    if (active && motor.get_gate()) {
      // ランプ処理
      if (m1 < m1_pulse) {
        m1 += _MOTOR_RAMP;
        if (m1 > m1_pulse) m1 = m1_pulse;
      } else if (m1 > m1_pulse) {
        m1 -= _MOTOR_RAMP;
        if (m1 < m1_pulse) m1 = m1_pulse;
      }
      if (m2 < m2_pulse) {
        m2 += _MOTOR_RAMP;
        if (m2 > m2_pulse) m2 = m2_pulse;
      } else if (m2 > m2_pulse) {
        m2 -= _MOTOR_RAMP;
        if (m2 < m2_pulse) m2 = m2_pulse;
      }
      // モータへ指令
      motor.set_duty (m1, m2);
    } else {
      m1 = m2 = 0;
    }
    vTaskDelayUntil (&t, 1);
  }
}

//---------------------------------------------------------------------------------
// 土俵際の回避タスク
//---------------------------------------------------------------------------------
void TASK3 (void *pvParameters) {
  uint32_t timeout;

  for (;;) {
    if (motor.get_gate() && (sen.RR || sen.RL)) {
      active = false;

      timeout = UD5_GET_ELAPSEDTIME() + 300;  // 最大0.3秒間だけ回避動作
      // 土俵センサが無反応もしくはタイムアウトするまでバック
      do {
        motor.set_duty (sen.RR ? -1000 : 0, sen.RL ? -1000 : 0);
        UD5_WAIT (1);
      } while ((timeout > UD5_GET_ELAPSEDTIME()) && (sen.RL || sen.RR));
      motor.set_duty (0, 0);

      active = true;
    }
    UD5_WAIT (1);
  }
}

//---------------------------------------------------------------------------------
// メイン
//---------------------------------------------------------------------------------
int main (void) {
  // 各タスクを作成
  xTaskCreate (TASK1, NULL, 200, NULL, 1, NULL);
  xTaskCreate (TASK2, NULL, 200, NULL, 1, NULL);
  xTaskCreate (TASK3, NULL, 100, NULL, 1, NULL);
  // カーネル起動
  vTaskStartScheduler();
}
