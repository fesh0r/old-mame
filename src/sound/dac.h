#ifndef DAC_H
#define DAC_H

#ifdef __cplusplus
extern "C" {
#endif

void DAC_data_w(int num,int data);
void DAC_signed_data_w(int num,int data);
void DAC_data_16_w(int num,int data);
void DAC_signed_data_16_w(int num,int data);

WRITE8_HANDLER( DAC_0_data_w );
WRITE8_HANDLER( DAC_1_data_w );
WRITE8_HANDLER( DAC_0_signed_data_w );
WRITE8_HANDLER( DAC_1_signed_data_w );

#ifdef __cplusplus
}
#endif

#endif
