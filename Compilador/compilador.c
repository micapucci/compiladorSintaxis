#include <stdio.h> 
#include <string.h> 
#include <ctype.h> 



#define NUMESTADOS 15
#define NUMCOLS 13
#define TAMLEX 32+1 
#define TAMNOM 20+1



FILE * in;

typedef enum { 
	INICIO, FIN, LEER, ESCRIBIR, ID, CONSTANTE, PARENIZQ, PARENDER, 
	PUNTOYCOMA, COMA, ASIGNACION, SUMA, RESTA, FDT, ERRORLEXICO
} TOKEN;

typedef struct{
	char identifi[TAMLEX];
	TOKEN t; 
} RegTS;

RegTS TS[1000] = { {"inicio", INICIO}, {"fin", FIN}, {"leer", LEER}, {"escribir", ESCRIBIR}, {"$", 99} };

typedef struct{
	TOKEN clase;
	char nombre[TAMLEX];
	int valor;
} REG_EXPRESION; 

char buffer[TAMLEX];

TOKEN tokenActual;

int flagToken = 0; 



TOKEN scanner();
int columna(int c);
int estadoFinal(int e);
void Objetivo(void);
void Programa(void);
void ListaSentencias(void);
void Sentencia(void);
void ListaIdentificadores(void);
void Identificador(REG_EXPRESION * presul);
void ListaExpresiones(void);
void Expresion(REG_EXPRESION * presul);
void Primaria(REG_EXPRESION * presul);
void OperadorAditivo(char * presul);
REG_EXPRESION ProcesarCte(void);
REG_EXPRESION ProcesarId(void);
char * ProcesarOp(void);
void Leer(REG_EXPRESION in);
void Escribir(REG_EXPRESION out);
REG_EXPRESION GenInfijo(REG_EXPRESION e1, char * op, REG_EXPRESION e2);
void Match(TOKEN t);
TOKEN ProximoToken();
void ErrorLexico();
void ErrorSintactico();
void Generar(char * co, char * a, char * b, char * c);
char * Extraer(REG_EXPRESION * preg);
int Buscar(char * id, RegTS * TS, TOKEN * t);
void Colocar(char * id, RegTS * TS);
void Chequear(char * s);
void Comenzar(void);
void Terminar(void);
void Asignar(REG_EXPRESION izq, REG_EXPRESION der);


int main (int argc, char* argv[]) {

	
	TOKEN tok;
	char nomArchi[TAMNOM];
	int l;
	
	if (argc == 1) { 
		printf ("Debe ingresar el nombre del archivo fuente (en lenguaje micro) en la linea de comando\n");
		return -1; 
	}
	
	if ( argc != 2 ) { 
 		return -1;
	}

	strcpy(nomArchi, argv[1]); 
	l = strlen(nomArchi); 

	if ( l > TAMNOM ) {
 		printf("Nombre incorrecto del Archivo Fuente\n");
 		return -1;
	}

	if ( nomArchi[l-1] != 'm' || nomArchi[l-2] != '.' ) { 
 		printf("Nombre incorrecto del Archivo Fuente\n");
 		return -1;
	}
	
	if ( (in = fopen(nomArchi, "r") ) == NULL ) { 
 		printf("No se pudo abrir archivo fuente\n");
 		return -1;
	}
	
	Objetivo (); 
	fclose (in); 
	return 0;
}	


void Objetivo(void) {
	Programa();
	Match(FDT); 
	Terminar(); 
}

void Programa(void) {
	Comenzar(); 
	Match(INICIO); 
	ListaSentencias(); 
	Match(FIN); 
}

void ListaSentencias(void){
	Sentencia(); 
	while ( 1 ) 
	{
		switch ( ProximoToken() ){ 
			case ID : 
			case LEER : 
			case ESCRIBIR :	
				Sentencia();
				break;
			default : return;
		}
	}
}

void Sentencia(void){
	TOKEN tok = ProximoToken(); 
	REG_EXPRESION izq, der; 
	switch ( tok ){ 
	
		
		 case ID :
			 Identificador(&izq); 
			 Match(ASIGNACION); 
			 Expresion(&der); 
			 Asignar(izq, der); 
			 Match(PUNTOYCOMA); 
			 break;
			 
		
		 case LEER : 
			 Match(LEER);
			 Match(PARENIZQ); 
			 ListaIdentificadores(); 
			 Match(PARENDER);
			 Match(PUNTOYCOMA);
			 break; 
			 
		
		 case ESCRIBIR :
			 Match(ESCRIBIR);
			 Match(PARENIZQ);
			 ListaExpresiones();
			 Match(PARENDER);
			 Match(PUNTOYCOMA);
			 break; 
			 
		 
		 default : return;
	}
}

void ListaIdentificadores(void){
	TOKEN t;
	REG_EXPRESION reg;
	Identificador(&reg);
	Leer(reg);
	for ( t = ProximoToken(); t == COMA; t = ProximoToken() ){
		Match(COMA);
		Identificador(&reg);
		Leer(reg);
	} 
}

void Identificador(REG_EXPRESION * presul){
	Match(ID);
	*presul = ProcesarId(); 
}

void ListaExpresiones(void){
	TOKEN t;
	REG_EXPRESION reg;
	Expresion(&reg);
	Escribir(reg);
	for ( t = ProximoToken(); t == COMA; t = ProximoToken() ){
		Match(COMA);
		Expresion(&reg);
		Escribir(reg);
	} 
}

void Expresion(REG_EXPRESION * presul){
	REG_EXPRESION operandoIzq, operandoDer;
	char op[TAMLEX];
	TOKEN t;
	Primaria(&operandoIzq);
	for ( t = ProximoToken(); t == SUMA || t == RESTA; t = ProximoToken() ){
		OperadorAditivo(op);
		Primaria(&operandoDer);
		operandoIzq = GenInfijo(operandoIzq, op, operandoDer);
	}
	*presul = operandoIzq;
} 

void Primaria(REG_EXPRESION * presul){
	TOKEN tok = ProximoToken();
	switch ( tok ){
	
		case ID : 
			Identificador(presul);
			break;
		
		case CONSTANTE : 
			Match(CONSTANTE);
			*presul = ProcesarCte();
			break;
	
		case PARENIZQ : 
			Match(PARENIZQ);
			Expresion(presul);
			Match(PARENDER);
			break;
	
		default : 
			return;
	}
}

void OperadorAditivo(char * presul){
	TOKEN t = ProximoToken();
	if ( t == SUMA || t == RESTA ){
		 Match(t);
		 strcpy(presul, ProcesarOp());
	}else{ 
		ErrorSintactico(t);
	}	
} 

REG_EXPRESION ProcesarCte(void){
	REG_EXPRESION reg;
	reg.clase = CONSTANTE;
	strcpy(reg.nombre, buffer);
	sscanf(buffer, "%d", &reg.valor);
	return reg;
}


REG_EXPRESION ProcesarId(void){
	REG_EXPRESION reg;
	Chequear(buffer);
	reg.clase = ID;
	strcpy(reg.nombre, buffer);
	return reg;
}


char * ProcesarOp(void){
	return buffer;
}


void Leer(REG_EXPRESION in){
	Generar("Read", in.nombre, "Entera", "");
}


void Escribir(REG_EXPRESION out){
	Generar("Write", Extraer(&out), "Entera", "");
}


REG_EXPRESION GenInfijo(REG_EXPRESION e1, char * op, REG_EXPRESION e2){
	REG_EXPRESION reg;
	static unsigned int numTemp = 1;
	char cadTemp[TAMLEX] ="Temp&";
	char cadNum[TAMLEX];
	char cadOp[TAMLEX];
	if ( op[0] == '-' ){
	 strcpy(cadOp, "Restar");
	}
	if ( op[0] == '+' ){
		strcpy(cadOp, "Sumar");
	} 
	sprintf(cadNum, "%d", numTemp);
	numTemp++;
	strcat(cadTemp, cadNum);
	if ( e1.clase == ID) {
		Chequear(Extraer(&e1));
	}
	if ( e2.clase == ID) {
		Chequear(Extraer(&e2));
	}
	Chequear(cadTemp);
	Generar(cadOp, Extraer(&e1), Extraer(&e2), cadTemp);
	strcpy(reg.nombre, cadTemp);
	return reg;
}


void Match(TOKEN t){
if ( !(t == ProximoToken()) )
	{ErrorSintactico();
} 
flagToken = 0;
}

TOKEN ProximoToken(){
	if ( !flagToken ){
		tokenActual = scanner();
		if ( tokenActual == ERRORLEXICO ){
			ErrorLexico();
		} 
		flagToken = 1;
		if ( tokenActual == ID ) {
			Buscar(buffer, TS, &tokenActual);
		}
	}
	return tokenActual;
}

void ErrorLexico(){
	printf("Error Lexico\n");
}
void ErrorSintactico(){
	printf("Error Sintactico\n");
}

void Generar(char * co, char * a, char * b, char * c){
	printf("%s %s%c%s%c%s\n", co, a, ',', b, ',', c);
}

char * Extraer(REG_EXPRESION * preg){
	return preg->nombre;
}

int Buscar(char * id, RegTS * TS, TOKEN * t){
	int i = 0;
	while ( strcmp("$", TS[i].identifi) ){
		 if ( !strcmp(id, TS[i].identifi) ) {
			 *t = TS[i].t;
			 return 1;
		 }
		 i++;
	}
	return 0;
}

void Colocar(char * id, RegTS * TS){
	int i = 4;
	while ( strcmp("$", TS[i].identifi) ){
		i++;
	} 
	if ( i < 999 ){
	 strcpy(TS[i].identifi, id );
	 TS[i].t = ID;
	 strcpy(TS[++i].identifi, "$" );
	}
}

void Chequear(char * s){
	TOKEN t;
	if ( !Buscar(s, TS, &t) ){
		 Colocar(s, TS);
		 Generar("Declara", s, "Entera", "");
	}
}

void Comenzar(void){}

void Terminar(void){
	Generar("Detiene", "", "", "");
}

void Asignar(REG_EXPRESION izq, REG_EXPRESION der){
	Generar("Almacena", Extraer(&der), izq.nombre, "");
}

TOKEN scanner(){
int tabla[NUMESTADOS][NUMCOLS] = { { 1, 3, 5, 6, 7, 8, 9, 10, 11, 14, 13, 0,
14 },
 { 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
 { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 } };
int car;
int col;
int estado = 0;
int i = 0;
do
{
 car = fgetc(in);
 col = columna(car);
 estado = tabla[estado][col];
 if ( col != 11 )
 {
 buffer[i] = car;
 i++;
 }
}
while ( !estadoFinal(estado) && !(estado == 14) );
buffer[i] = '\0';
switch ( estado )
{
 case 2 : if ( col != 11 )
 {
 ungetc(car, in);
 buffer[i-1] = '\0';
 }
 return ID;
 case 4 : if ( col != 11 )
 {
 ungetc(car, in);
 buffer[i-1] = '\0';
 }
 return CONSTANTE;
 case 5 : return SUMA;
 case 6 : return RESTA;
 case 7 : return PARENIZQ;
 case 8 : return PARENDER;
 case 9 : return COMA;
 case 10 : return PUNTOYCOMA;
 case 12 : return ASIGNACION;
 case 13 : return FDT;
 case 14 : return ERRORLEXICO;
}
return 0;
}
int estadoFinal(int e)
{
if ( e == 0 || e == 1 || e == 3 || e == 11 || e == 14 ) return 0;
return 1;
}
int columna(int c)
{
if ( isalpha(c) ) return 0;
if ( isdigit(c) ) return 1;
if ( c == '+' ) return 2;
if ( c == '-' ) return 3;
if ( c == '(' ) return 4;
if ( c == ')' ) return 5;
if ( c == ',' ) return 6;
if ( c == ';' ) return 7;
if ( c == ':' ) return 8;
if ( c == '=' ) return 9;
if ( c == EOF ) return 10;
if ( isspace(c) ) return 11;
return 12;
}
