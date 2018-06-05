// ConsoleApplication2.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include <stdarg.h>
#include<errno.h>
#include "Compiler.h"

#pragma warning(disable: 4996)
#define SAFEALLOC(var,Type)  if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

void tkerr(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	//fprintf(stderr, "error in line %d: ",tk->line); //optional
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
	exit(-1);
}

void err(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
	exit(-1);
}
typedef struct _Token {
	int code;       // code (name)
	union {
		char *text;  // used for ID, CT_STRING (dynamically allocated)
		long int integer;  // used for CT_INT
		char character; //used for CT_CHAR
		double real;    // used for CT_REAL
	};
	//int line; //optional
	struct _Token *next; // link to the next token
}Token;
Token *tokens;
Token *lastToken;
Token *currentTk;
FILE *fp;
Token *consumedTk;

int crtDepth = 0;

//SYMBOLS

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };

enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };

enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

typedef struct _Symbol Symbol;
typedef struct {
	Symbol **begin; //the beginning of the symbols (or NULL)
	Symbol **end; //the position after the last symbol
	Symbol **after; //the position after the allocated space
}Symbols;
Symbols symbols;

typedef struct {
	int typeBase; //TB_*
	Symbol *s; //struct definition for TB_STRUCT
	int nElements; //>0 array of given size, 0=array without size, <0 non array
}Type;

void addVar(Token *tkName, Type *t);

typedef struct _Symbol {
	const char *name; //reference to the name stored in a token
	int cls; //CLS_*
	int mem; //MEM*_
	Type type;
	int depth;
	union {
		Symbols args; //used only for functions
		Symbols members; //used only for structs
	};
}Symbol;

Symbol *crtStruct;
Symbol *crtFunc;



void initSymbols(Symbols *symbols) {
	symbols->begin = NULL;
	symbols->end = NULL;
	symbols->after = NULL;
}

Symbol *addSymbol(Symbols *symbols, const char *name, int cls) {
	Symbol *s;
	if (symbols->end == symbols->after) { //create more room
		int count = symbols->after - symbols->begin;
		int n = count * 2; //double the room
		if (n == 0) n = 1; //needed for the initial case
		symbols->begin = (Symbol **)realloc(symbols->begin, n * sizeof(Symbol *));
		if (symbols->begin == NULL) err("not enough memory");
		symbols->end = symbols->begin + count;
		symbols->after = symbols->begin + n;
	}
	SAFEALLOC(s, Symbol);
	*symbols->end++ = s;
	printf("ADDED SYMBOL %s WITH CLASS %d and DEPTH %d\n", name, cls,crtDepth);
	s->name = name;
	s->cls = cls;
	s->depth = crtDepth;
	return s;
}

Symbol *findSymbol(Symbols *symbols, const char *name) {
	int size = (symbols->end) - (symbols->begin);
	size--;
	while (size >= 0) {
		if (strcmp(symbols->begin[size]->name, name) == 0) {
			return symbols->begin[size];
		}
		size--;
	}
	return NULL;
}

void addVar(Token *tkName, Type *t){
	Symbol *s;
	if (crtStruct) {
		if (findSymbol(&crtStruct->members, tkName->text))
			tkerr("symbol redefinition: %s", tkName->text);
		s = addSymbol(&crtStruct->members, tkName->text, CLS_VAR);
	}
	else if (crtFunc) {
		s = findSymbol(&symbols, tkName->text);
		if (s&&s->depth == crtDepth)
			tkerr("symbol redefinition: %s", tkName->text);
		s = addSymbol(&symbols, tkName->text, CLS_VAR);
		s->mem = MEM_LOCAL;
	}
	else {
		if (findSymbol(&symbols, tkName->text))
			tkerr("symbol redefinition: %s", tkName->text);
		//printf("ADDED VARIABLE %s in addVar \n", tkName->text);
		s = addSymbol(&symbols, tkName->text, CLS_VAR);
		s->mem = MEM_GLOBAL;
	}
	s->type = *t;
}

void deleteSymbolsAfter(Symbols *symbols, Symbol *symbol) {
	int cnt = 0, i;
	int size = symbols->end - symbols->begin;
	int found = size;
	for (cnt = 0; cnt < size; cnt++) {
		if ((symbols->begin[cnt]) == symbol) {
			found = cnt;
			for (cnt++; cnt < size; cnt++) {
				free(symbols->begin[cnt]);
			}
			symbols->end = symbols->begin + found + 1;
		}
	}
}

enum token_codes {
	//0   1      2     3       4     5    6   7    8       9       10    11
	ID, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE,
	//12      13       14       15
	CT_INT, CT_REAL, CT_CHAR, CT_STRING,
	//16     17         18    19    20        21        22    23
	COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC,
	//24   25   26   27   28   29   30  31   32      33     34     35    36      37       38
	ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
	//39 - EOF
	END
};



Token *addTk(int code)
{
	Token *tk;
	SAFEALLOC(tk, Token)
		tk->code = code;
	printf("%d ", code);
	//tk->line=line;
	tk->next = NULL;
	if (lastToken) {
		lastToken->next = tk;
	}
	else {
		tokens = tk;
	}
	lastToken = tk;
	return tk;
}

Token *addTk2(int code, char *value)
{
	Token *tk;
	SAFEALLOC(tk, Token)
		tk->code = code;
	if (code == ID || code == CT_STRING) {
		tk->text = (char*)malloc(strlen(value) * sizeof(char));
		strcpy(tk->text, value);
		printf("%s ", tk->text);
	}
	if (code == CT_INT) {
		if (strchr(value, 'x')) {
			//printf("\nVALUE IS %s\n", value);
			tk->integer = strtol(value, NULL, 16);
			//printf("TK INTEGER IS %ld\n", tk->integer);
		}
		else {
			tk->integer = atoi(value);//(char*)malloc(strlen(value) * sizeof(char));
		}					  //strcpy(tk->integer, atoi(value));
		printf("%d ", tk->integer);
	}
	if (code == CT_REAL) {
		tk->real = atof(value); //(char*)malloc(strlen(value) * sizeof(char));
								//strcpy(tk->real, value);
		printf("%f ", tk->real);
	}
	if (code == CT_CHAR) {
		//printf("VALUE OF CHAR IS %s\n", value);
		tk->character = value[0];//(char*)malloc(strlen(value) * sizeof(char));
								 //strcpy(tk->character, value);
		printf("%c ", tk->character);
	}
	//tk->line=line;
	tk->next = NULL;
	if (lastToken) {
		lastToken->next = tk;
	}
	else {
		tokens = tk;
	}
	lastToken = tk;
	return tk;
}

void showTokens();

int getNextToken()
{
	int state = 0;

	char ch = fgetc(fp);
	char *token_name = (char *)malloc(256);
	int currentIndex = 0; //dimension of our "rule"
	int flag = 0;

	if (ch == EOF) {
		showTokens();
		exit(1);
	}

	//Token *tk;
	while (1) { // infinite loop
		switch (state) {
		case 0: // transitions test for state 0
			if (isalpha(ch) || ch == '_') {
				strncpy(token_name, &ch, 1);
				currentIndex++;
				state = 1; //ID
			}
			else if (isspace(ch)) { //consume the spaces
				int un_c = ch;
				while (isspace(un_c)) {
					un_c = fgetc(fp);
				}
				ungetc(un_c, fp);
				//pCrtCh++; // consume the character and remains in state 0
			}
			else if (ch == '\0' || ch == -1) {
				addTk(END);
				return END;
			}
			// O P E R A T O R S
			else if (ch == '+') {
				addTk(ADD);
				return ADD;
			}
			else if (ch == '=') {
				ch = fgetc(fp);
				if (ch == '=') { // we found == EQUAL
					addTk(EQUAL);
					return EQUAL;
				}
				else { // we found = ASSIGN
					ungetc(ch, fp);
					addTk(ASSIGN);
					return ASSIGN;
				}
			}
			else if (ch == '-') {
				addTk(SUB);
				return SUB;
			}
			else if (ch == '*') {
				addTk(MUL);
				return MUL;
			}
			else if (ch == '/') {
				state = 12;
			}
			else if (ch == '.') {
				addTk(DOT);
				return DOT;
			}
			else if (ch == '&') {
				ch = fgetc(fp);
				if (ch == '&') {
					addTk(AND);
					return AND;
				}
				ungetc(ch, fp);
				tkerr("Character expected: \'&\'");
			}
			else if (ch == '|') {
				ch = fgetc(fp);
				if (ch == '|') {
					addTk(OR);
					return OR;
				}
				ungetc(ch, fp);
				tkerr("Character expected: \'|\'");
			}
			else if (ch == '>') {
				ch = fgetc(fp);
				if (ch == '=') {
					addTk(GREATEREQ);
					return GREATEREQ;
				}
				ungetc(ch, fp);
				addTk(GREATER);
				return GREATER;
			}
			else if (ch == '<') {
				ch = fgetc(fp);
				if (ch == '=') {
					addTk(LESSEQ);
					return LESSEQ;
				}
				ungetc(ch, fp);
				addTk(LESS);
				return LESS;
			}
			else if (ch == '!') {
				ch = fgetc(fp);
				if (ch == '=') {
					addTk(NOTEQ);
					return NOTEQ;
				}
				ungetc(ch, fp);
				addTk(NOT);
				return NOT;
			}
			// END OF OPERATORS
			// ===================
			// D E L I M I T E R S
			else if (ch == ',') {
				addTk(COMMA);
				return COMMA;
			}
			else if (ch == ';') {
				addTk(SEMICOLON);
				return SEMICOLON;
			}
			else if (ch == '(') {
				addTk(LPAR);
				return LPAR;
			}
			else if (ch == ')') {
				addTk(RPAR);
				return RPAR;
			}
			else if (ch == '[') {
				addTk(LBRACKET);
				return LBRACKET;
			}
			else if (ch == ']') {
				addTk(RBRACKET);
				return RBRACKET;
			}
			else if (ch == '{') {
				addTk(LACC);
				return LACC;
			}
			else if (ch == '}') {
				addTk(RACC);
				return RACC;
			}
			//END OF DELIMITERS
			// ===================
			// C O N S T A N T S
			else if (isdigit(ch)) {
				strcpy(token_name, &ch);
				currentIndex++;

				if (ch != '0')
					state = 5; //3
				else if (ch == '0') {
					ch = fgetc(fp);

					if (ch >= '0' && ch <= '7') {
						strcpy(token_name + currentIndex, &ch);
						currentIndex++;
						state = 6; // OCTAL NR ? //5
					}
					else if (ch == 'x') {
						strcpy(token_name + currentIndex, &ch);
						currentIndex++;
						state = 7; // HEX NR ? //4
					}
					else if (ch == '.') {
						strcpy(token_name + currentIndex, &ch);
						currentIndex++;
						//ungetc(ch, fp);
						state = 8;
					}
					else { // end of number
						if (!isdigit(ch)) { //no longer CT_INT
							token_name[currentIndex] = '\0'; //form CT_INT
							ungetc(ch, fp);
							addTk2(CT_INT, token_name);
							return CT_INT;
						}
						else { // no other combination
							tkerr("Not int number");
						}
					}
				}
			}
			else if (ch == '\"') { // CT_STRING
				state = 11;
			}
			else if (ch == '\'') { // CT_CHAR
				state = 9;
			}
			break;
		case 1: // IMPLEMENT
			if (isalnum(ch) || ch == '_') {
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
			}
			else {
				ungetc(ch, fp);
				state = 10; // KEYWORD
			}
			break;
		case 2: //2
			if (ch != '*') state = 2; //we still have to consume
			else state = 4; //we are almost done
			break;
		case 3:
			if (ch != '\n' && ch != '\r' && ch != '\0' && ch != EOF) state = 3;
			else {
				ungetc(ch, fp);
				state = 0;
			}
			break;
		case 4:
			if (ch != '*' && ch != '/') state = 2;  //we are not done consuming comments
			else if (ch == '*') state = 4;
			else if (ch == '/') state = 0; // done
			break;
		case 5: // DECIMAL + REAL CASES
			if (isdigit(ch)) {
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
			}
			else if (ch == '.') {
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
				state = 8; // REAL CASE
			}
			else {
				if (ch == 'e' || ch == 'E') {
					//strcpy(token_name + currentIndex, &ch);
					//currentIndex++;
					ungetc(ch, fp);
					state = 8; // REAL CASE
				}
				else {
					ungetc(ch, fp);
					token_name[currentIndex] = '\0'; //formed a CT_INT
					addTk2(CT_INT, token_name);
					return CT_INT;
				}
			}
			break;
		case 6: // OCTAL
			if (ch >= '0' && ch <= '7') { //keep constructing
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
			}
			else { //no other case than 0 -> 7 => END
				token_name[currentIndex] = '\0';
				ungetc(ch, fp);
				addTk2(CT_INT, token_name);
				return CT_INT;
			}
			break;
		case 7: // HEX
			if (isdigit(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
			}
			else {
				token_name[currentIndex] = '\0';
				ungetc(ch, fp);
				addTk2(CT_INT, token_name);
				return CT_INT;
			}
			break;
		case 8: // REAL
			if (isdigit(ch)) {
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
			}
			else if (ch == 'e' || ch == 'E') {
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;

				ch = fgetc(fp);
				if ((ch == '-' || ch == '+') || isdigit(ch)) {
					strcpy(token_name + currentIndex, &ch);
					currentIndex++;
					char ch2 = fgetc(fp);
					printf("First is %c\n", ch2);
					while (isdigit(ch2)) {
						printf("\nFound %c after %c\n", ch2, ch);
						strcpy(token_name + currentIndex, &ch2);
						currentIndex++;
						ch2 = fgetc(fp);
					}
					ungetc(ch2, fp);
					token_name[currentIndex] = '\0';
					addTk2(CT_REAL, token_name);
					return CT_REAL;
				}
				else
					tkerr("Not valid exponent");
			}
			else {
				ungetc(ch, fp);
				token_name[currentIndex] = '\0';
				addTk2(CT_REAL, token_name);
				return CT_REAL;
			}
			break;
		case 9:
			if (ch == '\\') { //ESC
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
				ch = fgetc(fp);
				if (strchr("abfntrv\'?\"\\0", ch)) {
					strcpy(token_name + currentIndex, &ch);
					currentIndex++;
					ch = fgetc(fp);

					if (ch == '\'') {
						token_name[currentIndex] = '\0';
						addTk2(CT_CHAR, token_name);
						return CT_CHAR;
					}
					else tkerr("Invalid character before \'");
				}
				else tkerr("(CT_CHAR) Invalid character for \'");
			} // 1 - LETTER
			else {
				if (ch != '\'') {
					strcpy(token_name + currentIndex, &ch);
					currentIndex++;
					ch = fgetc(fp);
					if (ch == '\'') {
						addTk2(CT_CHAR, token_name);
						return CT_CHAR;
					}
					else
						tkerr("(CT_CHAR) Invalid character before \'");
				}
			}
			break;
		case 10:
			token_name[currentIndex] = '\0';
			ungetc(ch, fp);

			if (!strcmp("break", token_name)) { addTk(BREAK); return BREAK; }
			else if (!strcmp("char", token_name)) { addTk(CHAR); return CHAR; }
			else if (!strcmp("int", token_name)) { addTk(INT); return INT; }
			else if (!strcmp("void", token_name)) { addTk(VOID); return VOID; }
			else if (!strcmp("double", token_name)) { addTk(DOUBLE); return DOUBLE; }
			else if (!strcmp("struct", token_name)) { addTk(STRUCT); return STRUCT; }
			else if (!strcmp("while", token_name)) { addTk(WHILE); return WHILE; }
			else if (!strcmp("return", token_name)) { addTk(RETURN); return RETURN; }
			else if (!strcmp("if", token_name)) { addTk(IF); return IF; }
			else if (!strcmp("else", token_name)) { addTk(ELSE); return ELSE; }
			else if (!strcmp("for", token_name)) { addTk(FOR); return FOR; }
			else {
				addTk2(ID, token_name);
				return ID;
			}
			break;
		case 11:
			if (ch == '\\') {
				strcpy(token_name + currentIndex, &ch);
				currentIndex++;
				ch = fgetc(fp);
				if (strchr("abfnrtv'?\"\\0", ch)) {
					strcpy(token_name + currentIndex, &ch);
					currentIndex++;
				}
				else
					tkerr("Invalid character \'\\\'");
			}
			else {
				if (ch == '\"') {
					token_name[currentIndex] = '\0';
					addTk2(CT_STRING, token_name);
					return CT_STRING;
				}
				else {
					strcpy(token_name + currentIndex, &ch);
					currentIndex++;
				}
			}
			break;
		case 12:
			if (ch == '*') {
				state = 2;
			}
			else if (ch == '/') {
				state = 3;
			}
			else {// (ch != '*' && ch != '/') {
				ungetc(ch, fp);
				addTk(DIV);
				return DIV;
			}
			break;
		}

		ch = fgetc(fp);
		if (ch == EOF) flag++;
		if (flag > 2) {
			addTk(END);
			return END;
		}
	}
}



// PARSER

int rule_unit();        //DONE
int rule_declStruct();  //DONE
int rule_declVar();     //DONE
int rule_typeBase(Type *ret);    //DONE
int rule_arrayDecl(Type *ret);   //DONE
int rule_typeName(Type *ret);    //DONE
int rule_declFunc();    //DONE
int rule_funcArg();     //DONE
int rule_stm();         //DONE
int rule_stmCompound(); //DONE
int rule_expr();        //DONE
int rule_exprAssign();  //DONE
int rule_exprOr();      //DONE
int rule_exprOr_2();    //DONE
int rule_exprAnd();     //DONE
int rule_exprAnd_2();   //DONE
int rule_exprEq();      //DONE
int rule_exprEq_2();    //DONE
int rule_exprRel();     //DONE
int rule_exprRel_2();   //DONE
int rule_exprAdd();     //DONE
int rule_exprAdd_2();   //DONE
int rule_exprMul();     //DONE
int rule_exprMul_2();   //DONE
int rule_exprCast();    //DONE
int rule_exprUnary();   //DONE
int rule_exprPostfix(); //DONE
int rule_exprPostfix_2(); //DONE
int rule_exprPrimary(); //DONE

int consume(int code) {
	if (currentTk->code == code) {
		consumedTk = currentTk;
		currentTk = currentTk->next;
		return 1; // consumed => return true
	}
	return 0;  // don't consume it, position is unchanged => return false
}

int rule_unit() {
	while (1) {
		if (rule_declStruct()) {}
		else if (rule_declFunc()) {}
		else if (rule_declVar()) {}
		else break;
	}
	if (!consume(END)) tkerr("Missing END token");
	return 1;
}

int rule_declStruct() {
	Token *startTk = currentTk;
	if (!consume(STRUCT)) return 0;
	Token tkName = *currentTk;
	if (!consume(ID)) tkerr("Missing ID after STRUCT declaration.");
	if (!consume(LACC)) { currentTk = startTk; return 0; } //tkerr...
	//symbols part
	if (findSymbol(&symbols, tkName.text))
		tkerr("symbol redefinition: %s", tkName.text);
	crtStruct = addSymbol(&symbols, tkName.text, CLS_STRUCT);
	initSymbols(&crtStruct->members);
	//end of symbols part
	while (1) {
		if (rule_declVar()) {}
		else break;
	}
	if (!consume(RACC)) tkerr("Missing RACC after STRUCT declaration.");
	if (!consume(SEMICOLON)) tkerr("Missing SEMICOLON after STRUCT declaration.");
	crtStruct = NULL;
	return 1;
}

int rule_declVar() {
	Token *startTk = currentTk;
	Type t;
	Symbol *s;
	Token tkName;
	if (rule_typeBase(&t)) {
		Token tkName = *currentTk;
		if (consume(ID)) {
			if (!rule_arrayDecl(&t)) {
				t.nElements = -1;
			}
			addVar(&tkName, &t);
			while (1) {
				if (consume(COMMA)) {
					tkName = *currentTk;
					if (consume(ID)) {
						if (!rule_arrayDecl(&t)) {
							t.nElements = -1;
						}
						addVar(&tkName, &t);
					}
					else tkerr("Missing ID after COMMA in VAR declaration.");
				}
				else break;
			}
			if (consume(SEMICOLON)) return 1;
			else tkerr("Missing SEMICOLON after VAR declaration.");
		}
		else currentTk = startTk;
	} // while is done
	else currentTk = startTk;

	return 0;
}

int rule_typeBase(Type *ret) {
	Token *startTk = currentTk;
	if (consume(INT)) {
		ret->typeBase = TB_INT;
	}else if (consume(DOUBLE)) {
		ret->typeBase = TB_DOUBLE;
	}else if (consume(CHAR)) {
		ret->typeBase = TB_CHAR;
	}else if (consume(STRUCT)) {
		Token *tkName = currentTk;
		if (consume(ID)) {
				Symbol *s = findSymbol(&symbols, tkName->text);
				if (s == NULL)tkerr("undefined symbol: %s", tkName->text);
				if (s->cls != CLS_STRUCT)tkerr("%s is not a struct", tkName->text);
				ret->typeBase = TB_STRUCT;
				ret->s = s;
		}
		else tkerr("Missing ID after STRUCT.");
	}else {
		return 0;
	}
	return 1;
}

int rule_arrayDecl(Type *ret) {
	Token *startTk = currentTk;
	if (consume(LBRACKET)) {
		if (rule_expr()) {
			ret->nElements = 0; //for now we do not compute real size!!!!!!
		}
		if (consume(RBRACKET)) {
			return 1;
		}
		else tkerr("Missing RBRACKET after array declaration.");
	}
	currentTk = startTk;
	return 0;
}

int rule_typeName(Type *ret) {
	if (rule_typeBase(ret)) {
		if (!rule_arrayDecl(ret)) {
			ret->nElements = -1;
		}
		return 1;
	}
	return 0;
}

int rule_declFunc_helper() {
	Token *startTk = currentTk;

	if (consume(ID)) {
		if (consume(LPAR)) {
			if (rule_funcArg()) {
				while (1) {
					if (consume(COMMA)) {
						if (rule_funcArg()) {
							//return 1;
							continue;
						}
						else tkerr("Missing FUNC_ARG for function declaration.");
					}
					else break;
				}
			}
			if (consume(RPAR)) {
				if (rule_stmCompound()) {
					return 1;
				}
				else tkerr("Missing stmCompound for function declaration.");
			}
			else tkerr("Missing RPAR for function declaration.");
		}
	}
	currentTk = startTk;

	return 0;
}

int rule_declFunc() {
	Token *startTk = currentTk;
	Type t;
	Token tkName;
	if (rule_typeBase(&t)) {
		if (consume(MUL)) {
			t.nElements = 0;
		}
		else {
			t.nElements = -1;
		}
	}else if (consume(VOID)) {
		t.typeBase = TB_VOID;
	}
	else {
		return 0;
	}
	tkName = *currentTk;
	if (!(consume(ID))) {
		currentTk = startTk;
		return 0;
	}
	if (!consume(LPAR)) {
		currentTk = startTk;
		return 0;
	}
	if (findSymbol(&symbols, tkName.text))
		tkerr("symbol redefinition: %s", tkName.text);
	crtFunc = addSymbol(&symbols, tkName.text, CLS_FUNC);
	initSymbols(&crtFunc->args); //init a list for the arguments of the function
	crtFunc->type = t; //set function type as the one found prev
	crtDepth++; //increase depth
	if (rule_funcArg()) {
		while (1) {
			if (consume(COMMA)) {
				if (!rule_funcArg()) tkerr("Missing func arg in stm");
			}
			else break;
		}
	}
	if (!consume(RPAR)) tkerr("Missing RPAR in func declaration");
	crtDepth--;
	if (!rule_stmCompound()) tkerr("Missing compound statement");
	deleteSymbolsAfter(&symbols, crtFunc);
	crtFunc = NULL;
	return 1;
}

int rule_funcArg() {
	Token *startTk = currentTk;
	Type t;
	Token tkName;
	if (rule_typeBase(&t)) {
		tkName = *currentTk;
		if (consume(ID)) {
			if (!rule_arrayDecl(&t)) {
				t.nElements = -1;
			}
			Symbol  *s = addSymbol(&symbols, tkName.text, CLS_VAR);
			s->mem = MEM_ARG;
			s->type = t;
			s = addSymbol(&crtFunc->args, tkName.text, CLS_VAR);
			s->mem = MEM_ARG;
			s->type = t;
			return 1;
		}
		else tkerr("Missing ID for function argument.");
	}
	currentTk = startTk;
	return 0;
}

int rule_stm() {
	Token *startTk = currentTk;

	if (rule_stmCompound()) return 1;
	//IF LPAR expr RPAR stm (ELSE stm)?
	if (consume(IF)) {
		if (consume(LPAR)) {
			if (rule_expr()) {
				if (consume(RPAR)) {
					if (rule_stm()) {
						if (consume(ELSE)) {
							if (rule_stm()) return 1;
							else tkerr("Missing STM after else.");
						}
						return 1;
					}
					else tkerr("Missing STM declaration.");
				}
				else tkerr("Missing RPAR declaration.");
			}
			else tkerr("Missing EXPR declaration.");
		}
		else tkerr("Missing LPAR declaration.");
	}
	currentTk = startTk;

	//WHILE LPAR expr RPAR stm
	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (rule_expr()) {
				if (consume(RPAR)) {
					if (rule_stm()) {
						return 1;
					}
					else tkerr("Missing STM declaration.");
				}
				else tkerr("Missing RPAR declaration.");
			}
			else tkerr("Missing EXPR declaration.");
		}
		else tkerr("Missing LPAR declaration.");
	}
	currentTk = startTk;

	//FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
	if (consume(FOR)) {
		if (!consume(LPAR)) tkerr("Missing LPAR declaration.");
		rule_expr();
		if (!consume(SEMICOLON)) tkerr("Missing SEMICOLON declaration.");
		rule_expr();
		if (!consume(SEMICOLON)) tkerr("Missing SEMICOLON declaration.");
		rule_expr();
		if (!consume(RPAR)) tkerr("Missing RPAR declaration.");
		if (!rule_stm()) tkerr("Missing STM declaration.");
		return 1;
	}
	currentTk = startTk;

	//BREAK SEMICOLON
	if (consume(BREAK)) {
		if (consume(SEMICOLON))
			return 1;
		else tkerr("Missing SEMICOLON declaration.");
	}
	currentTk = startTk;

	//RETURN expr? SEMICOLON
	if (consume(RETURN)) {
		rule_expr();
		if (consume(SEMICOLON)) {
			return 1;
		}
		else tkerr("Missing SEMICOLON declaration.");
	}
	currentTk = startTk;

	//expr? SEMICOLON
	if (consume(SEMICOLON)) {
		return 1;
	}
	else { //CHANGED HERE
		if (rule_expr()) {
			if (!consume(SEMICOLON)) tkerr("Missing SEMICOLON declaration.");
			return 1;
		}
	}
	currentTk = startTk;
	return 0;
}

//LACC (declVar | stm)* RACC
int rule_stmCompound() {
	Symbol *start = symbols.end[-1];
	if (consume(LACC)) {
		crtDepth++;
		while (1) {
			if (rule_declVar()) continue;
			if (rule_stm())     continue;
			else break;
		}
		if (consume(RACC)) {
			crtDepth--;
			deleteSymbolsAfter(&symbols, start);
			return 1;
		}
		else tkerr("Missing RACC declaration.");
	}
	return 0;
}

int rule_expr() {
	Token *startTk = currentTk;

	if (rule_exprAssign())
		return 1;

	return 0;
}

//exprUnary ASSIGN exprAssign | exprOR
int rule_exprAssign() {
	Token *startTk = currentTk;
	if ((currentTk->code == NOT) || (currentTk->code == SUB)) {
		if (rule_exprUnary()) {
			if (consume(ASSIGN)) {
				if (rule_exprAssign())
					return 1;
				else tkerr("Missing exprAssign declaration.");
			}
		}
	}
	if (rule_exprPostfix()) {
		if (consume(ASSIGN)) {
			if (rule_exprAssign()) {
				return 1;
			}
			tkerr("Missing exprAssign.");
		}

	}
	currentTk = startTk;
	if (rule_exprOr()) return 1;
	currentTk = startTk;
	return 0;
}

int rule_exprOr_2() {
	Token *startTk = currentTk;

	if (consume(OR)) {
		if (rule_exprAnd()) {
			if (rule_exprOr_2()) {
				return 1;
			}
			else tkerr("Incomplete OR expression.");
		}
		else tkerr("Missing AND expression after OR declaration.");
	}

	currentTk = startTk;

	return 1;
}

int rule_exprOr() {
	Token *startTk = currentTk;

	if (rule_exprAnd()) {
		if (rule_exprOr_2())
			return 1;
		else tkerr("Incomplete OR expression.");
	}

	return 0;
}

int rule_exprAnd_2() {
	Token *startTk = currentTk;

	if (consume(AND)) {
		if (rule_exprEq()) {
			if (rule_exprAnd_2()) {
				return 1;
			}
			else tkerr("Missing rule_exprAnd_2.");
		}
		else tkerr("Missing expression after AND.");
	}
	currentTk = startTk;

	return 1;
}

int rule_exprAnd() {
	Token *startTk = currentTk;

	if (rule_exprEq()) {
		if (rule_exprAnd_2())
			return 1;
		else tkerr("Incomplete EQ expression.");
	}

	return 0;
}

int rule_exprEq_2() {
	Token *startTk = currentTk;

	if (consume(EQUAL)) {
		if (rule_exprRel()) {
			if (rule_exprEq_2()) return 1;
			else tkerr("Missing EXPR expr");
		}
		else tkerr("Missing REL expression.");
	}

	if (consume(NOTEQ)) {
		if (rule_exprRel()) {
			if (rule_exprEq_2()) return 1;
			else tkerr("Missing EXPR epr");
			return 1;
		}
		else tkerr("Missing REL expression.");
	}
	currentTk = startTk;

	return 1;
}

int rule_exprEq() {
	Token *startTk = currentTk;

	if (rule_exprRel()) {
		if (rule_exprEq_2())
			return 1;
		else tkerr("Incomplete EQ expression.");
	}

	return 0;
}

int rule_exprRel_3() {
	if (consume(LESS))      return 1;
	if (consume(LESSEQ))    return 1;
	if (consume(GREATER))   return 1;
	if (consume(GREATEREQ)) return 1;
	return 0;
}

int rule_exprRel_2() {
	Token *startTk = currentTk;

	if (rule_exprRel_3()) {
		if (rule_exprAdd()) {
			if (rule_exprRel_2()) return 1;
			else tkerr("Missing exprRel");
		}
		else tkerr("Missing ADD expression.");
	}
	currentTk = startTk;

	return 1;
}

int rule_exprRel() {
	Token *startTk = currentTk;

	if (!rule_exprAdd()) return 0;
	if (!rule_exprRel_2()) tkerr("Incomplete REL expression.");
	return 1;
}

int rule_exprAdd_2() {
	Token *startTk = currentTk;

	if (consume(ADD)) {
		if (rule_exprAdd()) {
			if (rule_exprAdd_2()) return 1;
			else tkerr("Missing expr after ADD.");
		}
		else tkerr("Missing REL expression.");
	}

	if (consume(SUB)) {
		if (rule_exprAdd()) {
			if (rule_exprAdd_2()) return 1;
			else tkerr("Missing expr after SUB.");
		}
		else tkerr("Missing REL expression.");
	}
	currentTk = startTk;

	return 1;
}

int rule_exprAdd() {
	Token *startTk = currentTk;

	if (rule_exprMul()) {
		if (rule_exprAdd_2())
			return 1;
		else tkerr("Incomplete ADD expression.");
	}

	return 0;
}

int rule_exprMul_2() {
	Token *startTk = currentTk;

	if (consume(MUL)) {
		if (rule_exprCast()) {
			if (rule_exprMul_2()) return 1;
			else tkerr("Missing mul after cast.");
		}
		else tkerr("Missing REL expression.");
	}

	if (consume(DIV)) {
		if (rule_exprCast()) {
			if (rule_exprMul_2()) return 1;
			else tkerr("Missing mul after cast.");
		}
		else tkerr("Missing REL expression.");
	}
	currentTk = startTk;

	return 1;
}

int rule_exprMul() {
	Token *startTk = currentTk;

	if (rule_exprCast()) {
		if (rule_exprMul_2())
			return 1;
		else tkerr("Incomplete MUL expression.");
	}

	return 0;
}

//LPAR typeName RPAR exprCast | exprUnary
int rule_exprCast() {
	Token *startTk = currentTk;
	Type t;
	if (consume(LPAR)) {
		if (rule_typeName(&t)) {
			if (consume(RPAR)) {
				if (rule_exprCast()) {
					return 1;
				}
				else tkerr("Incomplete CAST declaration.");
			}
			else tkerr("Missing RPAR expression after cast declaration.");
		}
		else tkerr("Missing TYPENAME expression after LPAR declaration.");
	}
	if (rule_exprUnary()) {
		return 1;
	}
	currentTk = startTk;

	return 0;
}

// (SUB | NOT) exprUnary | exprPostfix
int rule_exprUnary() {
	Token *startTk = currentTk;

	if (consume(SUB)) {
		if (rule_exprUnary())
			return 1;
		else tkerr("Missing UNARY expression.");
	}

	if (consume(NOT)) {
		if (rule_exprUnary())
			return 1;
		else tkerr("Missing UNARY expression.");
	}
	currentTk = startTk;
	if (rule_exprPostfix()) {
		return 1;
	}
	currentTk = startTk;

	return 0;
}

int rule_exprPostfix_2() {
	Token *startTk = currentTk;
	if (consume(LBRACKET)) {
		if (rule_expr()) {
			if (consume(RBRACKET)) {
				if (rule_exprPostfix_2()) return 1;
				else tkerr("Incomplete EXPRESSION declaration.");
			}
			else tkerr("Missing RBACKET after expression.");
		}
		else tkerr("Missing EXPRESSION after LBRACKET.");
	}
	if (consume(DOT)) {
		if (consume(ID)) {
			if (rule_exprPostfix_2()) return 1;
			else tkerr(" Incomplete EXPRESSION declaration.");
		}
		else tkerr("Missing ID in postfix expression.");
	}
	currentTk = startTk;
	return 1;
}

int rule_exprPostfix() {
	Token *startTk = currentTk;
	if (rule_exprPrimary()) {
		if (rule_exprPostfix_2()) {
			return 1;
		}
		else tkerr("Incomplete POSTFIX declaration.");
	}
	currentTk = startTk;
	return 0;
}

int rule_exprPrimary() {
	Token *startTk = currentTk;
	if (consume(ID)) {
		if (consume(LPAR)) {
			if (rule_expr()) {
				while (1) {
					if (consume(COMMA)) {
						if (rule_expr()) continue;
						else tkerr("Missing expression after ','.");
					}
					break;
				}
			}
			if (consume(RPAR)) {
				return 1;
			}
			else tkerr("Missing RPAR in primary EXPR after ID.");
		}
		return 1;
	}
	if (consume(CT_INT)) return 1;

	if (consume(CT_REAL)) return 1;

	if (consume(CT_CHAR)) return 1;

	if (consume(CT_STRING)) return 1;

	if (consume(LPAR)) {
		if (rule_expr()) {
			if (consume(RPAR))return 1;
			else tkerr("Missing RPAR in primary EXPR.");
		}
		else tkerr("Missing expression after LPAR.");
	}

	currentTk = startTk;
	return 0;
}


void start_lexor(Token *startTk) {
	currentTk = startTk;
	if (rule_unit()) printf("Sucessfully completed the syntactical analysis stage.\n");
	else tkerr("Error at ASYN");
}

// END OF PARSER
void showTokens() {
	Token* p = tokens;
	while (p) {
		printf("(%d", p->code);
		if (p->code == ID || p->code == CT_CHAR || p->code == CT_STRING) printf(":%s)", p->text);
		else if (p->code == CT_INT) printf(":%ld)", p->integer);
		else if (p->code == CT_REAL) printf(":%lf)", p->real);
		printf(" ");
		p = p->next;
	}
	printf("\n");
}

int main(int argc, char ** argv) {
	if (argc < 2) {
		printf("Wrong nr. of params.");
		exit(1);
	}

	printf("Compiling file: %s\n\n", argv[1]);
	fp = fopen(argv[1], "r");
	if (!fp) err("Cannot open the source file. (%s)", argv[1]);

	//	int c;
	tokens = (Token *)malloc(1000 * sizeof(Token));
	while (getNextToken() != END);
	printf("\n\nSucessfully completed the lexical analysis stage.");
	printf("\n");

	Token* p = tokens;
	start_lexor(p);

	return 0;
}
