# include <msp430g2231.h>
# include <legacymsp430.h>

typedef unsigned char	uint8_t;
typedef unsigned int	uint16_t;
typedef int  			int16_t;

volatile uint8_t rounds = 120-1;
volatile uint8_t ticks=0;
uint8_t tps = 0;
volatile uint8_t stacked=0;

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD + WDTNMI + WDTNMIES;	// stop WDT, enable NMI hi/lo

	//dco= digital oscillator instead of crystal
	//vlo= very low frequency oscillator
	//aclk= alternate clock (not main clock)

	BCSCTL3 |= LFXT1S_2;		// use VLO as ACLK, ~12Khz
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO to 1MHz
	DCOCTL = CALDCO_1MHZ;

	//set the watch dog timer for 8ms, at 1mhz thats
	//at 1mhz watch dog divides by 64 (2^6)
	//WDTIS0 is input to multiplexer to tell it to use 2^6
	//1000000 / 2^6 (wdt divider) = 5^6, 15625hz
	//watch dog checks in every 15khz
	//125 ticks of 1mhz / 64 = 8ms

	// now we got tps as ACLK/64 counts in one sec
	// for 1/8 sec per interrupt we will need to set CCR0 to tps * 8
	CCR0  = 64-1;				// we want one count every 64 clock cycle
	CCTL0 = CCIE;				// CCR0 interrupt enabled
	TACTL = TASSEL_1 + MC_1;	// ACLK, upmode

    WDTCTL = WDT_MDLY_8 + WDTNMI + WDTNMIES;	// WDT at 8ms, NMI hi/lo
	IE1 |= WDTIE;					// Enable WDT interrupt
	_BIS_SR(GIE);					// enable interrupt

	// wait for VLO calibration be done
	// timer interrupt to reduce rounds from 122 to 0
	while (rounds);

	// save ticks per second value for later user calibration (not implemented yet)
	unsigned char use_tps = tps;
	CCR0 = (use_tps-0) * 8;


	//configure gpio pins 
	//the camera needs both focus and shutter
	//to be closed circuit in order to take the photo
	P1DIR |=  (BIT4); //1.4 output (focus)
	P1DIR |=  (BIT5); //1.5 output (shutter)
	P1DIR |=  (BIT6); //1.6 output (green led)


	// LPM0 (shut down the CPU) with interrupts enabled
	__bis_SR_register(CPUOFF | GIE);

	return 0;
}


//calibration interrupt function runs to see how many
//ticks per second are on the ACLK
interrupt(WDT_VECTOR) wdt_ISR(void) {
	rounds--;
	if (!rounds) {
		tps = ticks;
		IE1 &= ~WDTIE;				// Disable WDT interrupt
	}
}


//interval counting interrupt function
//stacked is the number of whole seconds 

interrupt(TIMERA0_VECTOR) TimerA0_ISR(void) {

	//every interrupt make sure we're not 
	//taking a photo - this also resets 
	//from previous photo (1/8s ago)
	P1OUT |=  (BIT5); //P1.5 high
	P1OUT |=  (BIT4); //P1.4 high

	P1OUT &= ~(BIT6); //P1.6 low

	ticks++;

	if ((ticks&0x07) == 0)  // 1/8 second
	//if (ticks%4==0) // 1/2 second
	//if (ticks%2==0) // 1/4 second
	//if (1)
	{
		//1 time interval has elapsed
		stacked++;
		
		if(stacked % 5 == 0 )
		{ //1 photo every X seconds
			
			//turn on led
			P1OUT |=  (BIT6); //P1.6 high
			//take a photo
			P1OUT &= ~(BIT4); //P1.4 low
			P1OUT &= ~(BIT5); //P1.5 low
		}
	}
}
