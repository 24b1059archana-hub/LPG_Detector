/*==========================================================
 * LPG Gas Detector - AT89C51 @ 11.0592MHz
 * LCD: LM016L 8-bit on P1 | RS=P2.4 RW=P2.5 EN=P2.6
 * ADC: START=P2.0 ALE=P2.1 OE=P2.2 ADDA=P2.3
 *      EOC=P3.2 CLK=P3.5 DATA=P0
 * Buzzer=P3.7 | Fan(L293D IN1)=P3.4
 * GSM: P3.0/P3.1 @ 9600
 *==========================================================*/

#include <reg51.h>
#include <math.h>

sbit ADC_START = P2^0;
sbit ADC_ALE   = P2^1;
sbit ADC_OE    = P2^2;
sbit ADC_ADDA  = P2^3;
sbit LCD_RS    = P2^4;
sbit LCD_RW    = P2^5;
sbit LCD_EN    = P2^6;
sbit ADC_EOC   = P3^2;
sbit FAN       = P3^4;
sbit ADC_CLK   = P3^5;
sbit BUZZER    = P3^7;

#define RL_KOHM   20.0f
#define VCC_V      5.0f
#define RO_VALUE  10.0f    /* update after clean-air calibration */
#define FAN_PPM   200u
#define PHONE_NUM "\"9876543210\""

bit gsm_sent = 0;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++)
        for(j=0; j<110; j++);
}

/*=== ADC0808 ===*/
void adc_clk(unsigned char n) {
    while(n--) { ADC_CLK=1; ADC_CLK=0; }
}

unsigned char adc_read(unsigned char ch) {
    unsigned char val;
    ADC_ADDA  = ch & 1;
    ADC_ALE   = 1; adc_clk(2); ADC_ALE   = 0;
    ADC_START = 1; adc_clk(2); ADC_START = 0;
    while(ADC_EOC == 1);
    adc_clk(80);
    while(ADC_EOC == 0);
    ADC_OE = 1; val = P0; ADC_OE = 0;
    return val;
}

/*=== MQ-6 PPM ===*/
unsigned int gas_to_ppm(unsigned char read) {
		float vout, rs, ppm;
		unsigned char raw;
		raw = ((read & 0x80) >> 7) * 1   +
          ((read & 0x40) >> 6) * 2   +
          ((read & 0x20) >> 5) * 4   +
          ((read & 0x10) >> 4) * 8   +
          ((read & 0x08) >> 3) * 16  +
          ((read & 0x04) >> 2) * 32  +
          ((read & 0x02) >> 1) * 64  +
          ((read & 0x01) >> 0) * 128;
    
		
    if(raw == 0) return 0;
    vout = (raw / 255.0f) * VCC_V;
    if(vout >= VCC_V) return 0;
    rs   = RL_KOHM * (VCC_V - vout) / vout;
    ppm  = 970.0f * (float)pow((double)(rs / RO_VALUE), -2.48);
    if(ppm < 0)    return 0;
    if(ppm > 9999) return 9999;
    return (unsigned int)ppm;
}

/*=== Threshold pot: 0V=0PPM 5V=1000PPM ===*/
unsigned int pot_to_ppm(unsigned char read) {
		unsigned char raw;
		raw = ((read & 0x80) >> 7) * 1   +
          ((read & 0x40) >> 6) * 2   +
          ((read & 0x20) >> 5) * 4   +
          ((read & 0x10) >> 4) * 8   +
          ((read & 0x08) >> 3) * 16  +
          ((read & 0x04) >> 2) * 32  +
          ((read & 0x02) >> 1) * 64  +
          ((read & 0x01) >> 0) * 128;
    return (raw * 1000u) / 255u;
}

/*=== LCD ===*/
void lcd_en_pulse(void) { LCD_EN = 1; LCD_EN = 0; }

void lcd_cmd(unsigned char c) {
    LCD_RS=0; LCD_RW=0; P1=c;
    lcd_en_pulse(); delay_ms(2);
}

void lcd_char(unsigned char c) {
    LCD_RS=1; LCD_RW=0; P1=c;
    lcd_en_pulse();
}

void lcd_str(const char *s) { while(*s) lcd_char(*s++); }

void lcd_pos(unsigned char r, unsigned char c) {
    lcd_cmd(r ? (0xC0 | c) : (0x80 | c));
}


void lcd_num4(unsigned int v)
{
    if(v > 9999) v = 9999;   // limit value

    lcd_char((v/1000) + '0');
    lcd_char((v/100)%10 + '0');
    lcd_char((v/10)%10 + '0');
    lcd_char(v%10 + '0');
}

void lcd_init(void) {
    delay_ms(50);
    lcd_cmd(0x38); delay_ms(5);
    lcd_cmd(0x38); delay_ms(1);
    lcd_cmd(0x38);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01); delay_ms(2);
}

/*=== UART / GSM ===*/
void uart_init(void) {
    SCON=0x50; TMOD|=0x20;
    TH1=TL1=0xFD; TR1=1; TI=1;
}

void uart_ch(unsigned char c) { while(!TI); TI=0; SBUF=c; }
void uart_str(const char *s)  { while(*s) uart_ch(*s++); }

void uart_num(unsigned int v) {
    char b[5]; unsigned char l=0; signed char i;
    if(!v) { uart_ch('0'); return; }
    while(v) { b[l++]='0'+v%10; v/=10; }
    for(i=l-1; i>=0; i--) uart_ch(b[i]);
}

void gsm_sms(unsigned int gas, unsigned int thr) {
    uart_str("AT\r\n");         delay_ms(500);
    uart_str("AT+CMGF=1\r\n"); delay_ms(500);
    uart_str("AT+CMGS=");
    uart_str(PHONE_NUM);
    uart_str("\r\n");           delay_ms(1000);
    uart_str("LPG ALERT! Gas:"); uart_num(gas);
    uart_str(" PPM Threshold:"); uart_num(thr);
    uart_str(" PPM");
    uart_ch(0x1A);
    delay_ms(3000);
}

/*=== Main ===*/
void main(void) {
    unsigned char gas_raw, th_raw;
    unsigned int  gas_ppm, th_ppm;

    lcd_init();
    uart_init();
    ADC_ALE=ADC_START=ADC_OE=ADC_CLK=0;
    BUZZER=0; FAN=0;

    lcd_pos(0,0); lcd_str("  LPG DETECTOR  ");
    lcd_pos(1,0); lcd_str("  Initialising  ");
    delay_ms(1500);
    lcd_cmd(0x01); delay_ms(2);

    while(1) {
        gas_raw = adc_read(0);
        th_raw  = adc_read(1);

        gas_ppm = gas_to_ppm(gas_raw);
        th_ppm  = pot_to_ppm(th_raw);

        FAN = (gas_ppm >= FAN_PPM) ? 1 : 0;

        if(gas_ppm >= th_ppm) {
            BUZZER = 1;
            if(!gsm_sent) { gsm_sms(gas_ppm, th_ppm); gsm_sent=1; }
        } else {
            BUZZER=0; gsm_sent=0;
        }

        /* Row 0: "GAS:XXXX PPM" - always 16 chars */
        lcd_pos(0,0);
        lcd_str("GAS:");
        lcd_num4(gas_ppm);
        lcd_str(" PPM");

        /* Row 1: "TH: XXXX SAFE" - always 16 chars */
        lcd_pos(1,0);
        lcd_str("TH: ");
        lcd_num4(th_ppm);
        if(BUZZER)   lcd_str(" ALRT");
        else if(FAN) lcd_str(" WARN");
        else         lcd_str(" SAFE");

        delay_ms(500);
    }
}