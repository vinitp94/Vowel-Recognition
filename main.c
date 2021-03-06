#include "L138_LCDK_aic3106_init.h"
#include "L138_LCDK_switch_led.h"
#include "evmomapl138_gpio.h"
#include <math.h>
#include <string.h>

#define N 1024.000
#define PI 3.14159265358979
#define alpha 0.46
#define beta 0.54
#define K 513
#define M 26
#define Z 13

float frame1[1024];
float input;
float frame1_hamming[1024];
float psd[K];
float filterbank[M][K];
float energy[M];
float MFCC[Z];
float MFCCah[Z];
float MFCCeh[Z];
float MFCCee[Z];
float MFCCoo[Z];

int sw5;
int sw6;
int sw7;
int sw8;
int ahcount = 0;
int ehcount = 0;
int eecount = 0;
int oocount = 0;
int whbreak = 0;
float dist_ah;
float dist_eh;
float dist_ee;
float dist_oo;
char ah[3] = {'a', 'h','\0'};
char eh[3] = {'e', 'h','\0'};
char ee[3] = {'e', 'e','\0'};
char oo[3] = {'o', 'o','\0'};
char prov_vowel[3] = "";
char corr_vowel[3] = "";
int flag = 0;
float realsum = 0;
float imagsum = 0;
int counter = 0;
int temp = 0;
float fmel[M+2];
float fHz[M+2];
float fmap[M+2];
float mel_diff;
float kfloat;
float lfloat;
int m;
int i;
int j;
int k;
int l;
int check;
int user;

interrupt void interrupt4(void){
  sw5 = LCDK_SWITCH_state(5);
  sw6 = LCDK_SWITCH_state(6);
  sw7 = LCDK_SWITCH_state(7);
  sw8 = LCDK_SWITCH_state(8);
  
  if (sw5 == 0 && sw6 == 0 && sw7 == 0 && sw8 == 0){
    LCDK_LED_off(4);
    LCDK_LED_off(5);
    LCDK_LED_off(6);
    LCDK_LED_off(7);
    output_left_sample(0);
    temp = counter;
  }
  else{
    if (sw5 == 1 && ((sw6 + sw7 + sw8) == 0)){
      if (counter < 8000+temp){
        LCDK_LED_on(4);
        LCDK_LED_off(5);
        LCDK_LED_off(6);
        LCDK_LED_off(7);
        counter++;
      }
      else if (counter < 16000+temp){
        LCDK_LED_off(4);
        LCDK_LED_on(5);
        counter++;
      }
      else if (counter < 24000+temp){
        LCDK_LED_off(5);
        LCDK_LED_on(6);
        counter++;
        m = 0;
      }
      else if (counter < 25024+temp){
        LCDK_LED_off(6);
        LCDK_LED_on(7);
        float input = input_sample();
        if(m<8000){
          frame1[m] = input;
          m++;
        }
        counter++;
      }
      else{
        LCDK_LED_off(7);
        counter++;
        flag = 1;
      }
      output_left_sample(0);
    }
    return;
  }
}

int main(void){

  	L138_initialise_intr(FS_16000_HZ,ADC_GAIN_24DB,DAC_ATTEN_0DB,LCDK_MIC_INPUT);
  	LCDK_GPIO_init();
  	LCDK_SWITCH_init();
  	LCDK_LED_init();

  	fmel[27] = (float)(2595.000*log10(1.000+(8000.000/700.000)));
  	fmel[0] = (float)(2595.000*log10(1.000+(170.000/700.000)));
  	mel_diff = (fmel[M+1] - fmel[0])/(M+1);

  	for(i = 1; i < (M+1); i++){
  		fmel[i] = fmel[i-1] + mel_diff;
  	}
  	for(i = 0; i < (M+2); i++){
  		fHz[i] = (float)(((pow(10.0000,(fmel[i]/2595.000))) - 1.000)*700.000);
  	}
  	for(i = 0; i < (M+2); i++){
  		fmap[i] = round((fHz[i]-154.707)/15.29);
  	}

  	for(i = 1; i < (M+1); i++){
  		for (k = 0; k < K; k++){
  			if(k < fmap[i-1] || k > fmap[i+1]){
  				filterbank[i-1][k] = 0;
  			}
  			else if(k >= fmap[i-1] && k < fmap[i]){
  				filterbank[i-1][k] = ((k-fmap[i-1])/(fmap[i]-fmap[i-1]));
  			}
  			else if(k >= fmap[i] && k <= fmap[i+1]){
  				filterbank[i-1][k] = ((fmap[i+1]-k)/(fmap[i+1]-fmap[i]));
  			}
  		}
  	}
  	for (i = 0; i < Z; i++){
    		MFCCah[i] = 0.000;
    		MFCCeh[i] = 0.000;
    		MFCCee[i] = 0.000;
    		MFCCoo[i] = 0.000;
  	}

  	while(1){
  		if (flag == 1){
  			for (k = 0; k < 1024; k++){
  				kfloat = (float)(k);
  				frame1_hamming[k] = frame1[k]*(alpha - (beta*(cos((2.000*PI*(kfloat+1.000))/(N-1.000)))));
  			}

  			for (k = 0; k < K; k++){
  				kfloat = (float)(k);
  				for (l = 0; l < 1024; l++){
  					lfloat = (float)(l);
  					realsum += frame1_hamming[l]*cos((-2.000*PI*kfloat*(lfloat+1.000))/N);
  					imagsum += frame1_hamming[l]*sin((-2.000*PI*kfloat*(lfloat+1.000))/N);
  				}
  				psd[k] = (((realsum*realsum) + (imagsum*imagsum))/N);
  				realsum = 0;
  				imagsum = 0;
  			}

  			for (i = 0; i < M; i++){
  				energy[i] = 0.000;
  			}

  			for (i = 0; i < M; i++){
  				for (k = 0; k < K; k++){
  					energy[i] += (filterbank[i][k]*psd[k]);
  				}
  				energy[i] = log10(energy[i]);
  			}

  			for (i = 0; i < Z; i++){
  				MFCC[i] = 0.000;
  			}

  			for (i = 0; i < Z; i++){
  				for (m = 1; m < (M+1); m++){
  					MFCC[i] += energy[m]*cos(((i+1)*(float)(m-0.5))*(PI/20));
  				}
  			}

  			dist_ah = 0;
  			dist_eh = 0;
  			dist_ee = 0;
  			dist_oo = 0;

  			for (i = 0; i < Z; i++){
  				dist_ah += pow((MFCC[i] - MFCCah[i]), 2);
  				dist_eh += pow((MFCC[i] - MFCCeh[i]), 2);
  				dist_ee += pow((MFCC[i] - MFCCee[i]), 2);
  				dist_oo += pow((MFCC[i] - MFCCoo[i]), 2);
  			}
  			dist_ah = pow(dist_ah, 0.5);
  			dist_eh = pow(dist_eh, 0.5);
  			dist_ee = pow(dist_ee, 0.5);
  			dist_oo = pow(dist_oo, 0.5);

  			if (dist_ah <= dist_eh && dist_ah <= dist_oo && dist_ah <= dist_ee){
  				strcpy(prov_vowel, ah);
  			}
  			else if(dist_eh < dist_ah && dist_eh < dist_ee && dist_eh < dist_oo){
  				strcpy(prov_vowel, eh);
  			}
  			else if(dist_ee < dist_ah && dist_ee < dist_eh && dist_ee < dist_oo){
  				strcpy(prov_vowel, ee);
  			}
  			else{
  				strcpy(prov_vowel, oo);
  			}

  			printf("The spoken vowel is %s. \n", prov_vowel);
  			printf("Is this correct? Enter '0' for no or '1' for yes. \n");
  			scanf("%d", &check);

  			while (whbreak == 0){
  				if (check == 1){
  					strcpy(corr_vowel, prov_vowel);
  					whbreak = 1;
  				}
  				else if (check == 0){
  					printf("Please enter the correct vowel. Enter '0' for 'ah', '1' for 'eh', '2' for 'ee', or '3' for 'oo'. \n");
  					scanf("%d", &user);
  					if (user == 0){
  						strcpy(corr_vowel, "ah");
  					}
  					else if (user == 1){
  						strcpy(corr_vowel, "eh");
  					}
  					else if (user == 2){
  						strcpy(corr_vowel, "ee");
  					}
  					else if (user == 3){
  						strcpy(corr_vowel, "oo");
  					}
  					else{
  						printf("Please re-record input. \n");
  						strcpy(corr_vowel, "");
  					}
  					whbreak = 1;
  				}
  				else{
  					printf("Please re-record input. \n");
  					strcpy(corr_vowel, "");
  					whbreak = 1;
  				}
  			}

  			if (strcmp(corr_vowel, "ah") == 0){
  				ahcount++;
  				for (i = 0; i < Z; i++){
            MFCCah[i] = (((MFCCah[i]*(ahcount - 1)) + MFCC[i])/ahcount);
  				}
  			}

  			if (strcmp(corr_vowel, "eh") == 0){
  				ehcount++;
  				for (i = 0; i < Z; i++){
            MFCCeh[i] = (((MFCCeh[i]*(ehcount - 1)) + MFCC[i])/ehcount);
  				}
  			}

  			if (strcmp(corr_vowel, "ee") == 0){
  				eecount++;
  				for (i = 0; i < Z; i++){
            MFCCee[i] = (((MFCCee[i]*(eecount - 1)) + MFCC[i])/eecount);
  				}
  			}

  			if (strcmp(corr_vowel, "oo") == 0){
  				oocount++;
  				for (i = 0; i < Z; i++){
            MFCCoo[i] = (((MFCCoo[i]*(oocount - 1)) + MFCC[i])/oocount);
  				}
  			}

  			whbreak = 0;
  			counter = 0;
  			temp = 0;
  			flag = 0;
  		}

  		LCDK_GPIO_init();
  		LCDK_SWITCH_init();
  		LCDK_LED_init();
  		sw5 = LCDK_SWITCH_state(5);
  		sw6 = LCDK_SWITCH_state(6);
  		sw7 = LCDK_SWITCH_state(7);
  		sw8 = LCDK_SWITCH_state(8);

  		if (sw5 == 0 && sw6 == 0 && sw7 == 0 && sw8 == 0){
        L138_initialise_intr(FS_16000_HZ,ADC_GAIN_24DB,DAC_ATTEN_0DB,LCDK_MIC_INPU T);
  		}
  	}
}
