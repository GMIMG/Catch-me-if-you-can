
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define F_CPU 16000000UL 
#include <util/delay.h> 

#define TRIGER_PIN 0    //E0
#define ECHO_PIN 4      //D4  ICP1
#define ENABLE_CAPTURE_INTR (TIMSK|=(1<<TICIE1))
#define DISABLE_CAPTURE_INTR (TIMSK&= ~(1<<TICIE1))
#define TIMRER1_PRESCALE_64 (TCCR1B|=((1<<CS11)|(1<<CS10)|(1<<CS01)))
#define TIMRER1_CAPTURE_RISING (TCCR1B|=(1<<ICES1))
#define TIMRER1_CAPTURE_FALLING (TCCR1B&=~(1<<ICES1))

double getDistance();
void ULTRA();
void initialize();


volatile double distance=0.0;
typedef enum mode{TRIG,MEAS1,MEAS2, DISP} ultra;
volatile unsigned int t1=0,t2=0;

ultra mode=TRIG;


typedef enum {READY, RUN, ALARM} SET;
volatile unsigned int t=0;
SET state=READY;

unsigned char digit[10] = {0x77, 0x41, 0x3B, 0x5B, 0x4D, 0x5e, 0x7c, 0x43, 0x7f, 0x4f}; 
void disp_digit(unsigned char num, unsigned char d);
void FND();

int min=0, sec=0, K=0, pre_min=0x30, pre_sec=0x30;
unsigned char num;


	ISR(TIMER0_OVF_vect)
	{
		t++;
		if(state==RUN) if (t%1000==1) sec-=1;
		TCNT0=6;
	}

	ISR(INT0_vect)
	{
		switch (state){
			case READY: {state=RUN; PORTA&=~0x01; break;}
			case RUN: {state=READY; PORTA=0x01; min=0; sec=0; t=0; break;}
			case ALARM: {state=READY; PORTA=0x01; break;}
		}
	}


	ISR (TIMER1_CAPT_vect){
		if (mode==MEAS1){
			t1=ICR1;
			TIMRER1_CAPTURE_FALLING;
			mode=MEAS2;
		}
		else if  (mode==MEAS2) {
			t2=ICR1;
			DISABLE_CAPTURE_INTR;
			mode=DISP;
		}
	}




int main(void)
{
	initialize();

    while (1) 
    {
	ULTRA();

	if(distance>100)
	distance=99;
	if(distance<1)
	distance=1;

	FND();

		
	switch (state) {
	case READY:
		if (((PINB & 0x30)==0x10)&&(pre_min==0x30))
		min++;
		if (((PINB & 0x30)==0x20)&&(pre_min==0x30))
		sec++;
		PORTA=0x01;
		if (sec == 60)
		sec=0;
		if (min == 60)
		min=0;
		break;

	case RUN:
		{
		if((min==0)&&(sec==0))
		{
		state=ALARM;
		PORTA&=~0x3C;
		}
		if(state==RUN){
		if(sec==0)
		{sec=60; min-=1;}
		}
		break;
		}

	case ALARM:
		{
		//PORTA -	7	6	5	4	3	2	1	0
		//					d11	d12	d21	d22	spe	led


		//speaker와 50% dc모터 66%
		if(t%2==1)
		PORTA|=(0x02);
		else
		PORTA&=~(0x02);

		if(t%3==1)
		PORTA&=~(0x14);	
		else
		PORTA|=(0x14);
		
			if(distance<20){
			while (1) //방향바꾸기
			{
				if(t%100==3)
				K++;
				if((0<K)&&(K<700))
				PORTA|=0x28;//뒤로갔다가
				if((K<1220)&&(K>700))
				PORTA&=~0x08; //회전

				//speaker와 50% dc모터 50%
				if(t%2==1)
				PORTA|=((0x02)|(0x14));
				else
				PORTA&=~((0x02)|(0x14));

	
					FND();

				if(K>1220)
				{K=0;
				PORTA&=~0x28;//전진
				distance=20;
				break;
				}
			}
			}

		}
	break;
	}
	
			

	pre_min=(PINB & 0x30);
	pre_sec=(PINB & 0x30);
	}

}


 void disp_digit(unsigned char num, unsigned char d)
 {
	 PORTC = digit[num];
	 PORTF = 0x01<<d;
 }

 double getDistance(){
	 return (double)((t2-t1)*4)/58;    // result in terms of cm
 }

 void FND()
 {
	 if ((t % 10)==0) {
		 num= (sec)%10;
	 disp_digit(num, 3); }
	 if ((t % 10)==2) {
		 num= (sec/10)%10;
	 disp_digit(num, 2); }
	 if ((t % 10)==4) {
		 num= (min)%10;
	 disp_digit(num, 1); }
	 if ((t % 10)==6) {
		 num= (min/10)%10;
	 disp_digit(num, 0); }
 }

 void initialize()
 {

	 _delay_ms(100);

	 DDRD=0x00;
	 DDRE=0x01;
	 PORTE=0x00;
	 TIMRER1_PRESCALE_64;
	 TIMRER1_CAPTURE_RISING;
	 mode=TRIG;


	 DDRC = 0xff;
	 DDRF = 0x0f;
	 DDRB = 0x00;
	 DDRA = 0xff;

	 EICRA|=(1<<ISC01);
	 EIMSK|=(1<<INT0);

	 TCCR0 |= 1<<CS02; // Prescale 64  4us
	 TIMSK |= 1<<TOIE0; // Interrupt Enable
	 TCNT0=6; // 4us x (256 - 6) = 1ms
	 sei();

	 distance=50;
	 PORTA |= 0x01;
 }

 void ULTRA()
 {

	 if (mode==TRIG){
		 PORTE|=1<<TRIGER_PIN;
		 _delay_us(10);
		 PORTE&=~(1<< TRIGER_PIN);
		 _delay_us(5);
		 mode=MEAS1;
		 t1=0.;t2=0.;
		 TIMRER1_CAPTURE_RISING;
		 ENABLE_CAPTURE_INTR;
	 }
	 else if (mode==DISP){
		 distance=0.7*distance+0.3*getDistance();
		 mode=TRIG;
	 }
 }