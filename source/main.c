#include <16F877a.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#use delay (clock = 20000000)
#fuses HS, NOWDT, NOPROTECT, NOPUT, NOBROWNOUT, NOLVP
#use rs232 (baud = 9600, bits = 8, parity = N, uart1, errors) //configurações para a porta serial xmit = pin_c6, rcv = pin_c7
// 9600 8N1

#define BUFFER_SIZE 30

typedef struct packet
{
   int16 posicao;
   unsigned int16 tempo_pos;
   signed int16 angulo;
   unsigned int16 tempo_ang;
   int8 estado_carro;
   int8 estado_pendulo;
}ReceivePacket;

typedef enum{
   ESPERANDO_H,
   ESPERANDO_POS,
   ESPERANDO_TEMPO_POS,
   ESPERANDO_ANG,
   ESPERANDO_TEMPO_ANG,
   ESPERANDO_ESTADO_CARRO,
   ESPERANDO_ESTADO_PENDULO
}EstadoRecepcao;

typedef enum{
   ESTADO_ESPERANDO_PACOTE,
   ESTADO_RECEBENDO_PACOTE
}EstadoPacote;

typedef struct {
   float lower_bound_l;   // Mais baixa do trapézio
   float lower_bound_h;   // Mais alta do trapézio
   float upper_bound_h;   // Mais alta do trapézio
   float upper_bound_l;
} Function;

#define   RULE_NUMBER   3

//#define   QL   0
#define   SL   0
#define   S   1
#define SR   2
//#define QR   4

//#define   BN   0
#define SN   0
#define   Z   1
#define SP   2
//#define BP   4

Function theta_input_rules[RULE_NUMBER] = {
   //{-225,-150,-150,-75},

   {-150,-75,-75,0},
   {-75,0,0,75},
   {0,75,75,150},
   //{75,150,150}
};

Function dtheta_input_rules[RULE_NUMBER] = {
   //{-15,-10,-10,-5},

   {-10,-5,-5,0},
   {-5,0,0,5},
   {0,5,5,10},
   //{5,10,10,15}
};

Function output_rules[RULE_NUMBER] = {
   //{-150,-100,-100,-50},

   {-100,-50,-50,0},
   {-50,0,0,50},
   {0,50,50,100},
   //{50,100,100,150}
};

float activation_vector[RULE_NUMBER] = {0.0f, 0.0f, 0.0f};

float And (float x, float y) {
   if (x>y)
      return y;
   return x;
}

float Max (float x, float y) {
   if (x>y)
      return x;
   return y;
}

float mu_of (float x, Function * fuzzy) {
   float result,a,b;

   ////printf("\r\nx %f",x);
   ////printf("\r\nfuzzy->lower_bound_l %ld",fuzzy->lower_bound_l);
   ////printf("\r\nfuzzy->upper_bound_l %ld",fuzzy->upper_bound_l);

   if (x < fuzzy->lower_bound_l)   {
      ////printf("\n x < fuzzy->lower_bound_l");
      return 0.0f;//fuzzy->value_lower_bound;
   }

   if (x > fuzzy->upper_bound_l){
     ////printf("\n x > fuzzy->lower_bound_l");
      return 0.0f;//fuzzy->value_upper_bound;
   }

   if (x >= fuzzy->lower_bound_l && x <= fuzzy->lower_bound_h){
      result=(x - fuzzy->lower_bound_l)/(fuzzy->lower_bound_h - fuzzy->lower_bound_l);
      //printf("\r\nresult1 %f  ", result);
      return result;
   }
   if (x >= fuzzy->upper_bound_h && x <= fuzzy->upper_bound_l) {
      a=fuzzy->upper_bound_l - x;
      b=fuzzy->upper_bound_l - fuzzy->upper_bound_h;
      result=a/b;
      //printf("\r\nresult2 %f [%f,%f] ", result, a, b);
      return result;
   }

   if (x >= fuzzy->lower_bound_h && x <= fuzzy->upper_bound_h){
      ////printf("\n x >= fuzzy->lower_bound_h && x <= fuzzy->upper_bound_h %f  ", result);
      return 1.0f;
   }


   return -1.0f;
}

void EvaluateRules (float theta, float dtheta) {
   float mu_theta[RULE_NUMBER];
   float mu_dtheta[RULE_NUMBER];
   float res;
   int i;

   for (i = 0; i < RULE_NUMBER; i++) {
         mu_theta[i] = mu_of(theta, &theta_input_rules[i]);
         ////printf("\r\n[%d] %f %f ", i, mu_theta[i], theta);
         mu_dtheta[i] = mu_of(dtheta, &dtheta_input_rules[i]);
         ////printf("\r\n[%d] %f %f ", i, mu_dtheta[i], dtheta);
         activation_vector[i] = 0.0f;
   }

   res = And (mu_theta[SN], mu_dtheta[SN]);
   ////printf("muthetaZ %f mudthetaZ %f ", mu_theta[SN], mu_dtheta[SN]);
   activation_vector[SL] = Max (activation_vector[SL], res);
   ////printf("\r\nres1 %f ", res);

   res = And (mu_theta[SN], mu_dtheta[Z]);
   ////printf("muthetaZ %f mudthetaZ %f ", mu_theta[SN], mu_dtheta[Z]);
   activation_vector[SL] = Max (activation_vector[SL], res);
   ////printf("\r\nres2 %f ", res);

   res = And (mu_theta[SN], mu_dtheta[SP]);
   ////printf("muthetaZ %f mudthetaZ %f ", mu_theta[SN], mu_dtheta[SP]);
   activation_vector[SL] = Max (activation_vector[SL], res);
   ////printf("\r\nres3 %f ", res);

   // Bloco 3
   res = And (mu_theta[Z], mu_dtheta[SN]);
   //printf("muthetaZ %f mudthetaZ %f ", mu_theta[Z], mu_dtheta[SN]);
   activation_vector[S] = Max (activation_vector[S], res);
   //printf("\r\nres4 %f ", res);

   res = And (mu_theta[Z], mu_dtheta[Z]);
   //printf("muthetaZ %f mudthetaZ %f ", mu_theta[Z], mu_dtheta[Z]);
   activation_vector[S] = Max (activation_vector[S], res);
   //printf("\r\nres5 %f ", res);

   res = And (mu_theta[Z], mu_dtheta[SP]);
   //printf("muthetaZ %f mudthetaZ %f activation_vector[S] %f res %f", mu_theta[Z], mu_dtheta[SP], activation_vector[S], res);
   activation_vector[S] = Max (activation_vector[S], res);
   //printf("\r\nres6 %f ", res);

   // Bloco 4

   res = And (mu_theta[SP], mu_dtheta[SN]);
   //printf("muthetaZ %f mudthetaZ %f ", mu_theta[SP], mu_dtheta[Z]);
   activation_vector[SR] = Max (activation_vector[SR], res);
   //printf("\r\nres7 %f ", res);

   res = And (mu_theta[SP], mu_dtheta[Z]);
   //printf("muthetaZ %f mudthetaZ %f ", mu_theta[SP], mu_dtheta[Z]);
   activation_vector[SR] = Max (activation_vector[SR], res);
   //printf("\r\nres8 %f ", res);

   res = And (mu_theta[SP], mu_dtheta[SP]);
   //printf("muthetaZ %f mudthetaZ %f ", mu_theta[SP], mu_dtheta[SP]);
   activation_vector[SR] = Max (activation_vector[SR], res);
   //printf("\r\nres9 %f ", res);
   }

float Defuzzify (void) {

   float numerator, denominator;
   unsigned int i;

   numerator = denominator = 0.0f;
   for (i = 0; i < RULE_NUMBER; i++) {
      numerator += (activation_vector[i] * (output_rules[i].lower_bound_h + output_rules[i].upper_bound_l) / 2.0f);
      denominator += activation_vector[i];
   }
   ////printf("\r\n numerator %f denominator %f ",numerator, denominator);
   if(denominator==0){
      return 0;
   }

   return (float)(numerator/denominator);
}

void decode();


BOOLEAN recebeuTudo;
int indexString;
char string_recebida[BUFFER_SIZE];
volatile ReceivePacket pacote;
EstadoPacote estado = ESTADO_ESPERANDO_PACOTE;

char numeroRecebido[5];
int indiceNumeroRecebido;
EstadoRecepcao estadoAtual = ESPERANDO_H;
char temp;

#int_rda
void reception ()
{
   while (kbhit()) {
      temp = getc();

     if(estado == ESTADO_ESPERANDO_PACOTE){
         if(temp == 'H'){
            estado = ESTADO_RECEBENDO_PACOTE;
         }
     }

     if(estado == ESTADO_RECEBENDO_PACOTE){
                // Se o buffer estiver cheio descarta os bytes lidos
         if (indexString < BUFFER_SIZE)
          string_recebida[ indexString++ ] = temp;

         if(temp == '\n'){
            decode();
            indexString = 0;
            recebeuTudo = TRUE;

            estado = ESTADO_ESPERANDO_PACOTE;
         }
     }
  }

}
//O protocolo é: H,<pos_carro>,<tempo_carro>,<angulo>,<tempo_angulo>,<estado_carro>,<estado_pendulo>\n
void decode(){

   char i, temp;




   for(i = 0 ; i < indexString ; i++){
      temp = string_recebida[i];

      if(estadoAtual == ESPERANDO_H){
         if(temp == ','){
            estadoAtual = ESPERANDO_POS;
         }
      }else if(estadoAtual == ESPERANDO_POS){
         if(temp == ','){
         numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.posicao = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_POS;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_POS){
         if(temp == ','){
            //numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            //pacote.tempo_pos = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ANG;
         }else{
            //numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ANG){
         if(temp == ','){
                  numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.angulo = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_ANG;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_ANG){
         if(temp == ','){
            //numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            //pacote.tempo_ang = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_CARRO;
         }else{
            //numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_CARRO){
         if(temp == ','){
                 numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.estado_carro = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_PENDULO;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_PENDULO){
         if(temp == '\n'){
            numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.estado_pendulo = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            recebeuTudo = TRUE;
            estadoAtual = ESPERANDO_H;
            indexString = 0;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }
   }

}

//signed int16 velocity = 0;
//signed int16 previousVelocity = 0;
//signed int16  n;
//signed int16 alfa;
//signed int16 beta;
//int16 direcao;

signed int16 angulo_prev = 0;
signed int16 power=0;
signed int16 oldPower=0;

void main ()
{
   enable_interrupts(global);     //habilita a interrupção global
   enable_interrupts(int_rda);    //habilita a interrupção de recepção de caractere pela porta serial

   pacote.posicao = -1;
   pacote.tempo_pos = -1;
   pacote.angulo = -1;
   pacote.tempo_ang = -1;
   pacote.estado_carro = -1;
   pacote.estado_pendulo = -1;

   while (true)            //loop de repetição do código principal
   {



      if(recebeuTudo)
      {
/*        alfa = pacote.angulo % 2400;
        if(pacote.angulo < 0){
         alfa = 2400 - alfa;
        }
         beta = 300;

         if( (abs(alfa) < beta) || abs(alfa - 1200) < beta || abs(alfa - 2400) < beta){
            direcao = 0;
         }else if(alfa > 1200 && alfa < 2400){
            direcao = 1;
         }else if(alfa > 0 && alfa < 1200){
            direcao = -1;
         }

         velocity = 140 * direcao;
         if(velocity != previousVelocity){
                     ////printf("%ld\n", velocity);
                     previousVelocity = velocity;
                  }*/
         recebeuTudo = FALSE;

         EvaluateRules (pacote.angulo, pacote.angulo - angulo_prev);

         power=Defuzzify()*2;
         if(oldPower!=power){
            printf ("%ld\r\n", power);
            oldPower=power;
         }
         angulo_prev = pacote.angulo;
      }



}      // fim do main
   }   // fim do while
