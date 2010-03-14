/////////////////////////////////////////////////////////////////////////
// HEC2HRP.C  in machine
/*      Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c
        Hector 1
        Interact

        12/05/2009 Skeleton driver - Micko : mmicko@gmail.com
        31/06/2009 Video - Robbbert

        29/10/2009 Update skeleton to functional machine
                          by yo_fr       (jj.stac@aliceadsl.fr)

               => add Keyboard,
               => add color,
               => add cassette,
               => add sn76477 sound and 1bit sound,
               => add joysticks (stick, pot, fire)
               => add BR/HR switching
               => add bank switch for HRX
               => add device MX80c and bank switching for the ROM
       03/01/2010 Update and clean prog  by yo_fr       (jj.stac@aliceadsl.fr)
               => add the port mapping for keyboard

      don't forget to keep some information about these machines, see DChector project : http://dchector.free.fr/ made by DanielCoulom
      (and thank's to Daniel!)

    TODO : Add the cartridge function,
           Adjust the one shot and A/D timing (sn76477)
*/

#include "emu.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "sound/sn76477.h"   /* for sn sound*/
#include "sound/wave.h"      /* for K7 sound*/
#include "sound/discrete.h"  /* for 1 Bit sound*/
#include "deprecat.h"
#include "includes/hec2hrp.h"


static void Mise_A_Jour_Etat(int Adresse, int Value );
static void Update_Sound(const address_space *space, UINT8 data);

static running_device *cassette_device_image(running_machine *machine);

/* Stat for the register in 0x3000*/
static UINT8 state3000=0;
static UINT8 write_cassette=0;

/* Cassette timer*/
static TIMER_CALLBACK( Callback_CK );
static emu_timer *Cassette_timer;
static UINT8 CK_signal ;  /* Synchro signal for K7*/

/* Status for CPU clock*/
static UINT8 flag_clk=0;  /* 0 5MHz (HR machine) - 1  1.75MHz (BR machine)*/

static double Pin_Value[29][2]; /* SN76477*/
static int AU[17];              /* Pour les fils de bus audio du SN76477*/
static int ValMixer;            /* for mixer*/
static int oldstate3000;        /* Edge state mixer*/
static int oldstate1000;        /* Edge state cassette*/


/* joy map*/
static UINT8 pot0 = 0x40, pot1 = 0x40;  /* State for joy resistor*/
static UINT8 actions;					/* joy button off*/

static TIMER_CALLBACK( Callback_CK )
{
/* To generate the CK signal (K7)*/
	CK_signal++;
}

WRITE8_HANDLER( hector_switch_bank_w )
{
	if (offset==0x00)	{	/* 0x800 et 0x000=> video page, HR*/
                        	memory_set_bank(space->machine, "bank1", HECTOR_BANK_VIDEO);
							if (flag_clk ==1)
							{
        	    				flag_clk=0;
								cputag_set_clock(space->machine, "maincpu", XTAL_5MHz);  /* Augmentation CPU*/
							}
						}
	if (offset==0x04)	{	/* 0x804 => video page, BR*/
							hector_flag_hr=0;
                        	memory_set_bank(space->machine, "bank1", HECTOR_BANK_VIDEO);
							if (flag_clk ==0)
							{
								flag_clk=1;
								cputag_set_clock(space->machine, "maincpu", XTAL_1_75MHz);  /* Ralentissement CPU*/
							}
						}
	if (offset==0x08)	{	/* 0x808 => base page, HR*/
							memory_set_bank(space->machine, "bank1", HECTOR_BANK_PROG);
							if (flag_clk ==1)
							{
								flag_clk=0;
								cputag_set_clock(space->machine, "maincpu", XTAL_5MHz);  /* Augmentation CPU*/
							}

						}
	if (offset==0x0c)	{	/* 0x80c => base page, BR*/
							hector_flag_hr=0;
                        	memory_set_bank(space->machine, "bank1", HECTOR_BANK_PROG);
							if (flag_clk ==0)
							{
								flag_clk=1;
								cputag_set_clock(space->machine, "maincpu", XTAL_1_75MHz);  /* Ralentissement CPU*/
							}
						}
}

WRITE8_HANDLER( hector_keyboard_w )
{
	/*nothing to do => read function manage the value*/
}

READ8_HANDLER( hector_keyboard_r )
{
	UINT8 data = 0xff;
	running_machine *machine = space->machine;

	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7", "KEY8" };

	if (offset ==7) /* Only when joy reading*/
	{
		/* Read special key for analog joystick emulation only (button and pot are analog signal!) and the reset */
		data=input_port_read(machine, keynames[8]);

		if (data & 0x01) /* Reset machine ! (on ESC key)*/
		{
		  cputag_set_input_line(machine, "maincpu", INPUT_LINE_RESET, PULSE_LINE);
          if (!strncmp(machine->gamedrv->name , "hec2xxxx" , 4) |
              !strncmp(machine->gamedrv->name , "victor" , 6)     ) /* aviable for all HR machines*/
          {
               memory_set_bank(machine, "bank1", HECTOR_BANK_PROG);
               memory_set_bank(machine, "bank2", HECTORMX_BANK_PAGE0);
               hector_flag_hr=1;
          }
          else
               hector_flag_hr=0;
          /*Common flag*/
          hector_flag_80c = 0;
          flag_clk = 0;

		}

		actions = 0;
		if (data & 0x02) /* Fire(0)*/
			actions += 1;

		if (data & 0x04) /* Fire(1)*/
			actions += 2;

		if (data & 0x08) /* Pot(0)+*/
			pot0 += 1;
		if (pot0>128) pot0 = 128;

		if (data & 0x10) /* Pot(0)-*/
			pot0 -= 1;

		if (pot0>250) pot0 = 0;

		if (data & 0x20) /* Pot(1)+*/
			pot1 += 1;
		if (pot1>128) pot1 = 128;

		if (data & 0x40) /* Pot(1)-*/
			pot1 -= 1;

		if (pot1>250) pot1 = 0;
	}

	/* in all case return the request value*/
	return input_port_read(space->machine, keynames[offset]);
}

WRITE8_HANDLER( hector_sn_2000_w )
{
	   Mise_A_Jour_Etat( 0x2000+ offset, data);
	   Update_Sound(space, data);
}
WRITE8_HANDLER( hector_sn_2800_w )
{
	Mise_A_Jour_Etat( 0x2800+ offset, data);
	Update_Sound(space, data);
}
READ8_HANDLER( hector_cassette_r )
{
double level;
UINT8 value=0;
static UINT8 cassette_bit=0;
static UINT8 cassette_bit_mem=0;
static UINT8 Data_K7=0;

if ((state3000 & 0x38) != 0x38 )   /* Selon Sb choix cassette ou timer (74153)*/
{

   Data_K7 =  0x00;  /* No cassette => clear bit*/
       switch (state3000 & 0x38 )
       {
          case 0x08: value = (actions & 1) ? 0x80 : 0; break;
          case 0x10: value = pot0; break;
	      case 0x20: value = (actions & 2) ? 0x80 : 0; break;
          case 0x28: value = pot1; break;
          default: value = 0; break;
       }
}
else
{
	if (write_cassette == 0)
    {
		/* Accee a la cassette*/
		level = cassette_input(cassette_device_image(space->machine));

		/* Travail du 741 en trigger*/
		if  (level < -0.08)
    		cassette_bit = 0x00;
		if (level > +0.08)
			cassette_bit = 0x01;
	}
    /* Programme du sn7474 (bascule) : Changement ??tat bit Data K7 ?? chaque front montant de cassette_bit*/
    if ((cassette_bit != cassette_bit_mem) && (cassette_bit !=0))
    {
	    if (Data_K7 == 0x00)
		   Data_K7 =  0x80;/* En poids fort*/
	    else
		    Data_K7 =  0x00;
    }
	value = ( CK_signal & 0x7F ) + Data_K7;
    cassette_bit_mem = cassette_bit;  /* Memorisation etat bit cassette*/
}
return value;
}
WRITE8_HANDLER( hector_sn_3000_w )
{
    state3000 = data & 0xf8; /* except bit 0 to 2*/
	if ((data & 7) != oldstate3000 )
    	{
           /* Update sn76477 only when necessary!*/
           Mise_A_Jour_Etat( 0x3000, data & 7 );
	       Update_Sound(space, data & 7);
        }
        oldstate3000 = data & 7;
}

/* Color Interface */
WRITE8_HANDLER( hector_color_a_w )
{
static int counter_write=0; /* Attente de quelque cycles avant demettre en route*/

       if (data & 0x40)
	   {
		 /* Bit 6 => motor ON/OFF => for cassette state!*/
         if (write_cassette==0)
			{
			 cassette_set_state(cassette_device_image(space->machine) , (cassette_state)(CASSETTE_PLAY | CASSETTE_SPEAKER_ENABLED));
			}
       }
       else
       {	/* stop motor*/
           cassette_set_state(cassette_device_image(space->machine) , CASSETTE_STOPPED);
		   write_cassette=0;
		   counter_write =0;
       }
	   if (((data & 0x80) != (oldstate1000 & 0x80)) && ((oldstate1000 & 7)==(data & 7)) ) /* Bit7 had change but not the color statement*/
       {
		/* Bit 7 => Write bit for cassette!*/
		counter_write +=1;

		if (counter_write > 5)
		{
            /* Wait several cycle before lauch the record to prevent somes bugs*/
			counter_write = 6;
			if (write_cassette==0)
			{   /* C'est la 1er fois => record*/
				cassette_set_state(cassette_device_image(space->machine) , CASSETTE_RECORD );
				write_cassette=1;
			}
		}
		/* cassette data */
		cassette_output(cassette_device_image(space->machine), ((data & 0x80) == 0x80) ? -1.0 : +1.0);
	   }

        /* Other bit : color definition*/
	hector_color[0] =  data        & 0x07 ;
	hector_color[2] = ((data >> 3)  & 0x07) | (hector_color[2] & 0x40);

	oldstate1000=data; /* For next step*/
}
WRITE8_HANDLER( hector_color_b_w )
{
	running_device *discrete = devtag_get_device(space->machine, "discrete");
    	hector_color[1] =  data        & 0x07;
	hector_color[3] = (data >> 3)  & 0x07;

	/* Half light on color 2 only on HR machines:*/
	if (data & 0x40) hector_color[2] |= 8; else hector_color[2] &= 7;

	/* Play bit*/
	discrete_sound_w(discrete, NODE_01,  (data & 0x80) ? 0:1 );
}


/******************************************************************************
 Cassette Handling
******************************************************************************/

static running_device *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, "cassette");
}


/********************************************************************************
 Port Handling
********************************************************************************/

READ8_HANDLER( hector_mx_io_port_r)
{
   return 0;  // printer ok
}
WRITE8_HANDLER( hector_mx40_io_port_w)
{
   if ((offset &0x0ff) == 0xf0) /* Port A => to printer*/
		printer_output(devtag_get_device(space->machine, "printer"), data);
   if ((offset &0x0ff) == 0x40) /* Port page 0*/
      memory_set_bank(space->machine, "bank2", HECTORMX_BANK_PAGE0);
   if ((offset &0x0ff) == 0x41) /* Port page 1*/
   {
      memory_set_bank(space->machine, "bank2", HECTORMX_BANK_PAGE1);
      hector_flag_80c=0;
   }
   if ((offset &0x0ff) == 0x44) /* Port page 2  => 42 pour MX80*/
      memory_set_bank(space->machine, "bank2", HECTORMX_BANK_PAGE2);
   if ((offset &0x0ff) == 0x49) /* Port screen resolution*/
      hector_flag_80c=0;/* No 80c in 40c !*/
}

WRITE8_HANDLER( hector_mx80_io_port_w)
{
   if ((offset &0x0ff) == 0xf0) /* Port A => to printer*/
		printer_output(devtag_get_device(space->machine, "printer"), data);
   if ((offset &0x0ff) == 0x40) /* Port page 0*/
      memory_set_bank(space->machine, "bank2", HECTORMX_BANK_PAGE0);
   if ((offset &0x0ff) == 0x41) /* Port page 1*/
   {
      memory_set_bank(space->machine, "bank2", HECTORMX_BANK_PAGE1);
      hector_flag_80c=0;
   }
   if ((offset &0x0ff) == 0x42) /* Port page 2  => port different du MX40*/
      memory_set_bank(space->machine, "bank2", HECTORMX_BANK_PAGE2);
   if ((offset &0x0ff) == 0x49) /* Port screen resolution*/
      hector_flag_80c=1;
}

/********************************************************************************
 sound managment
********************************************************************************/

void Mise_A_Jour_Etat(int Adresse, int Value )
{
/* Adjust value depending on I/O main CPU request*/
 switch(Adresse )
 {
  case 0x2000:
   /* Modification AU0 / AU8*/
   {  /* AU0*/
      AU[ 0] =  ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU8 : 0*/
      AU[ 8] =  ((Value & 0x040 )==0) ? 0 : 1 ;
      break;
   }
  case 0x2001:
   /* Modification AU1 / AU9*/
   {  /* AU1*/
      AU[ 1] =  ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU9*/
      AU[ 9] =  ((Value & 0x040 )==0) ? 0 : 1 ;
      break;
   }
  case 0x2002:
   /* Modification AU2 / AU10*/
   {  /* AU2*/
      AU[ 2] =  ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU10*/
      AU[10] =  ((Value & 0x040 )==0) ? 0 : 1 ;
      break;
   }
   case 0x2003:
   /* Modification AU3 / AU11*/
   {  /* AU3*/
      AU[ 3] =  ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU11*/
      AU[11] =  ((Value & 0x040 )==0) ? 0 : 1 ;
      break;
   }
  case 0x2800:
   /* Modification AU4 / AU12*/
   {  /* AU4*/
      AU[ 4] =  ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU8*/
      AU[12] =  ((Value & 0x040 )==0) ? 0 : 1 ;
      break;
   }
  case 0x2801:
   /* Modification AU5 / AU13*/
   {  /* AU5*/
      AU[ 5] =  ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU13*/
      AU[13] =  ((Value & 0x040 )==0) ? 0 : 1 ;
     break;
   }
  case 0x2802:
   {  /* Modification AU6 / AU14*/
      /* AU6*/
      AU[ 6] = ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU14*/
      AU[14] = ((Value & 0x040 )==0) ? 0 : 1 ;
      break;
   }
  case 0x2803:
   /* Modification AU7 / AU15*/
   {  /* AU7*/
      AU[ 7] =  ((Value & 0x080 )==0) ? 0 : 1 ;
      /* AU15*/
      AU[15] =  ((Value & 0x040 )==0) ? 0 : 1 ;
      break;
   }
  case 0x3000:
   /* Mixer modification*/
   {
      ValMixer = (Value & 7) ;
       break;
   }
  default: break;

 } /*switch*/
}


static void Init_Value_SN76477_Hector(void)
{
     /* Remplissage des valeurs de resistance et capacite d'Hector*/

     /* Decay R*/
     Pin_Value[7][1] = RES_K(680.0); /*680K  */
	 Pin_Value[7][0] = RES_K(252.325); /* 142.325 (680 // 180KOhm)*/

     /* Capa A/D*/
     Pin_Value[8][0] = CAP_U(0.47); /* 0.47uf*/
     Pin_Value[8][1] = CAP_U(1.47);  /* 1.47*/

     /* ATTACK R*/
     Pin_Value[10][1]= RES_K(180.0);   /* 180*/
     Pin_Value[10][0]= RES_K(32.054); /* 32.054 (180 // 39 KOhm)*/

     /* Version 3 : Ajuste pour les frequences mesurees :
                // 4  0 SOUND 255 Hz => ajuste a l'oreille
                // 4  4 SOUND  65 Hz => ajuste a l'oreille
                // 4  8 SOUND  17 Hz =>  ajuste a l'oreille
                // 4 12 SOUND 4,3 Hz =>  ajuste a l'oreille*/
     /*   SLF C       Version 3*/
     Pin_Value[21][0]= CAP_U(0.1);  /*CAPU(0.1) */
     Pin_Value[21][1]= CAP_U(1.1);  /*1.1*/

     /*SLF R        Version 3*/
     Pin_Value[20][1]= RES_K(180);    //180 vu
     Pin_Value[20][0]= RES_K(37.268); //37.268 (47//180 KOhms)

     /* Capa VCO*/
     /* Version 3 : Ajust?? pour les frequences mesur??es :
             // 0 0  SOUND 5,5KHz => 5,1KHz
             // 0 16 SOUND 1,3KHz => 1,2KHz
             // 0 32 SOUND 580Hz  => 570Hz
             // 0 48 SOUND 132Hz  => 120Hz*/
     Pin_Value[17][0] = CAP_N(47.0) ;  /*47,0 mesure ok */
     Pin_Value[17][1] = CAP_N(580.0) ; /*580  mesure ok */
     /* R VCO   Version 3*/
     Pin_Value[18][1] = RES_K(1400.0   );/*1300 mesure ok    // au lieu de 1Mohm*/
     Pin_Value[18][0] = RES_K( 203.548 );/*223  mesure ok    // au lieu de 193.548 (1000 // 240KOhm)*/

     /* VCO Controle*/
     Pin_Value[16][0] = 0.0;  /* Volts  */
     Pin_Value[16][1] = 1.41; /* 2 =  10/15eme de 5V*/

     /* Pitch*/
     Pin_Value[19][0] = 0.0;   /*Volts */
     Pin_Value[19][1] = 1.41;


     Pin_Value[22][0] = 0; /* TOR */
     Pin_Value[22][1] = 1;

     /* R OneShot*/
     Pin_Value[24][1] = RES_K(100);
	 Pin_Value[24][0] = RES_K(1000);  /*RES_M(1) infini sur Hector car non connectee*/

     /* Capa OneShot*/
     Pin_Value[23][0] = 1.0;
     Pin_Value[23][1] = 0.0;  /* Valeur Bidon sur Hector car mise au 5Volts sans capa*/

     /* Enabled*/
     Pin_Value[9][0] = 0;
     Pin_Value[9][1] = 1;

     /* Volume*/
     Pin_Value[11][0] = 128; /* Rapport 50% et 100%  128*/
     Pin_Value[11][1] = 255; /*                      255*/

     /* Noise filter*/
     Pin_Value[6][0] = CAP_U(0.390);    /* 0.390*/
     Pin_Value[6][1] = CAP_U(08.60);    /* 0.48*/

     /* Valeur corrige par rapport au schema :*/
     Pin_Value[5][1] = RES_K(3.30 ) ;     /* 330Kohm*/
     Pin_Value[5][0] = RES_K(1.76 ) ;     /* 76 Kohm*/

     /* Noise pas commande par le bus audio !*/
	 /* Seule la valeur [0] est documentee !*/
     Pin_Value[4][0] = RES_K(47) ;      /* 47 K ohm*/
     Pin_Value[12][0] = RES_K(100);     /* 100K ohm*/
     Pin_Value[3][0] = 0 ;              /* NC*/

     /* Gestion du type d'enveloppe*/
     Pin_Value[ 1][0] = 0;
     Pin_Value[ 1][1] = 1;

     Pin_Value[28][0] = 0;
     Pin_Value[28][1] = 1;

     /* Initialisation a 0 des pin du SN*/
     AU[0]=0;
     AU[1]=0;
     AU[2]=0;
     AU[3]=0;
     AU[4]=0;
     AU[5]=0;
     AU[6]=0;
     AU[7]=0;
     AU[8]=0;
     AU[9]=0;
     AU[10]=0;
     AU[11]=0;
     AU[12]=0;
     AU[13]=0;
     AU[14]=0;
     AU[15]=0;
     ValMixer = 0;
}

void Update_Sound(const address_space *space, UINT8 data)
{
	/* keep device*/
	running_device *sn76477 = devtag_get_device(space->machine, "sn76477");

	/* MIXER*/
	sn76477_mixer_a_w(sn76477, ((ValMixer & 0x04)==4) ? 1 : 0);
	sn76477_mixer_b_w(sn76477, ((ValMixer & 0x01)==1) ? 1 : 0);
	sn76477_mixer_c_w(sn76477, ((ValMixer & 0x02)==2) ? 1 : 0);/* Revu selon mesure electronique sur HRX*/

    /* VCO oscillateur*/
    if (AU[12]==1)
        sn76477_vco_res_w(		sn76477, Pin_Value[18][AU[10]]/12.0); /* en non AU11*/
    else
        sn76477_vco_res_w(		sn76477, Pin_Value[18][AU[10]]); /* en non AU11*/

    sn76477_vco_cap_w(		sn76477, Pin_Value[17][AU[2 ]]);
    sn76477_pitch_voltage_w(sn76477, Pin_Value[19][AU[15]]);
    sn76477_vco_voltage_w(	sn76477, Pin_Value[16][AU[15]]);
    sn76477_vco_w(          sn76477, Pin_Value[22][AU[12]]); /* VCO Select Ext/SLF*/

   /* SLF*/
    sn76477_slf_res_w( sn76477, Pin_Value[20][AU[ 9]]);/*AU10*/
    sn76477_slf_cap_w( sn76477, Pin_Value[21][AU[1 ]]);

    /* One Shot*/
    sn76477_one_shot_res_w(sn76477, Pin_Value[24][     0]); /* NC*/
    sn76477_one_shot_cap_w(sn76477, Pin_Value[23][AU[13]]);

	/* Ampli value*/
    sn76477_amplitude_res_w(sn76477, Pin_Value[11][AU[5]]);

    /* Attack / Decay*/
    sn76477_attack_res_w(sn76477, Pin_Value[10][AU[ 8]]);
    sn76477_decay_res_w( sn76477, Pin_Value[7 ][AU[11]]);/*AU9*/
    sn76477_attack_decay_cap_w(sn76477, Pin_Value[8][AU[0]]);

    /* Filtre*/
    sn76477_noise_filter_res_w(sn76477, Pin_Value[5][AU[4]]);
    sn76477_noise_filter_cap_w(sn76477, Pin_Value[6][AU[3]]);

    /* Clock Extern Noise*/
    sn76477_noise_clock_res_w(sn76477, Pin_Value[4][0]); /* fix*/
    sn76477_feedback_res_w(sn76477, Pin_Value[12][0]);	/*fix*/

    /*  Envelope*/
    sn76477_envelope_1_w(sn76477, Pin_Value[1 ][AU[6]]);
    sn76477_envelope_2_w(sn76477, Pin_Value[28][AU[7]]);

	/* En dernier on lance (ou pas !)*/
    sn76477_enable_w(sn76477, Pin_Value[9][AU[14]]);
}

sn76477_interface hector_sn76477_interface =
{
	RES_K(47),		/*  4  noise_res*/
	RES_K(330),		/*  5  filter_res*/
	CAP_P(390),		/*  6  filter_cap*/
	RES_K(680),		/*  7  decay_res*/
	CAP_U(47),		/*  8  attack_decay_cap*/
	RES_K(180),		/* 10  attack_res*/
	RES_K(33),		/* 11  amplitude_res*/
	RES_K(100),		/* 12  feedback_res*/
	2,				/* 16  vco_voltage*/
	CAP_N(47) ,		/* 17  vco_cap*/
	RES_K(1000),	/* 18  vco_res*/
	2,				/* 19  pitch_voltage*/
	RES_K(180),		/* 20  slf_res*/
	CAP_U(0.1),		/* 21  slf_cap*/
	CAP_U(1.00001),	/* 23  oneshot_cap*/
    RES_K(10000)	/* 24  oneshot_res*/
};

void hector_reset(running_machine *machine, int hr)
{
	hector_flag_hr = hr;
	flag_clk = 0;
	write_cassette = 0;
}

void hector_init(running_machine *machine)
{
	/* For Cassette synchro*/
	Cassette_timer = timer_alloc(machine, Callback_CK, 0);
	timer_adjust_periodic(Cassette_timer, ATTOTIME_IN_MSEC(100), 0, ATTOTIME_IN_USEC(64));/* => real synchro scan speed for 15,624Khz*/

	/* Sound sn76477*/
	Init_Value_SN76477_Hector();  /*init R/C value*/
}
