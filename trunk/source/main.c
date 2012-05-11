#include <16F877a.h>
#include <stdio.h>
#use delay (clock = 20000000)
#fuses HS, NOWDT, NOPROTECT, NOPUT, NOBROWNOUT, NOLVP
#use rs232 (baud = 9600, bits = 8, stream=PEND, uart1, errors) //configurações para a porta serial xmit = pin_c6, rcv = pin_c7


typedef struct packet
{
   int posicao;
   int tempo_pos;
   int angulo;
   int tempo_ang;
   int estado_carro;
   int estado_pendulo;
}ReceivePacket;

BOOLEAN recebeuTudo;
int indexString;
char string_recebida[50];
ReceivePacket pacote;

#int_rda
void reception ()
{
  char temp;
  temp = getch();
  
  if(temp != '\n')
  {
   printf("\ti:%d c:%c |", indexString,temp);
   string_recebida[indexString++] = temp;
  }
  else
  {
   recebeuTudo = TRUE;
  }
}

//Assumindo string nnn
int StringToInt(char* string, int size)
{   
   char offset = '0';
   int value = 0;
   
    printf("E ");
   
   if(size == 3)
   {
      value += (string[0]-offset)*100;
      value += (string[1]-offset)*10;
      value += (string[2]-offset);
   }
   else if (size == 2)
   {
      value += (string[0]-offset)*10;
      value += (string[1]-offset);
   }
   else if (size == 1)
   {
      value += (string[0]-offset);
   }
   
   return value;
}

/*
*  Retorna um char[] com ["posicao", "tempo_pos", "angulo", "tempo_angulo", "estado_carro", "estado_pendulo"]
*/
char* SplitPacket(char* source)
{
   
   char* term, ptr, retorno[6];
   int i;
   
    printf("C ");
   
   term = ",";
   ptr = strtok(source, term); // Inicio do pacote "H"
   
   for(i=0; i<6; i++)
   {
      ptr = strtok(0, term);
      retorno[i] = *ptr;
   }
   
   return retorno;
}

void processPacket ()
{
   char dados[6];
   
   printf("B ");
   
   *dados = SplitPacket(string_recebida);
   
    printf("D ");
    
   pacote.posicao = StringToInt(dados[0], 3);
   pacote.tempo_pos = StringToInt(dados[1], 3);
   pacote.angulo = StringToInt(dados[2], 3);
   pacote.tempo_ang = StringToInt(dados[3], 3);
   pacote.estado_carro = StringToInt(dados[4], 3);
   pacote.estado_pendulo = StringToInt(dados[5], 3);
   
    printf("F ");
}

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
         printf(" A ");
        recebeuTudo = FALSE;
        processPacket();
      }
   
      //if(pacote.posicao >= 0)
      //{
         printf("%d, ", pacote.posicao);
         printf("%d, ", pacote.tempo_pos);
         printf("%d, ", pacote.angulo);
         printf("%d, ", pacote.tempo_ang);
         printf("%d, ", pacote.estado_carro);
         printf("%d\n", pacote.estado_pendulo);
      //}
      
      delay_ms (1000);
   }   // fim do while
}      // fim do main
