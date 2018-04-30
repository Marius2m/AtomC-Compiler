#include<stdio.h>
#include<stdlib.h>
#include <stdarg.h>

#define SAFEALLOC(var,Type)  if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");
FILE *fp;

enum token_codes{
//0   1      2     3       4     5    6   7    8       9       10    11
  ID, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE,
//12      13       14       15
  CT_INT, CT_REAL, CT_CHAR, CT_STRING,
//16     17         18    19    20        21        22    23
  COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC,
//24   25  26    27   28   29   30   31     32    33      34    35    36        37      38
  ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
//39 - EOF
  END
};

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


Token *addTk(int code)
{
  Token *tk;
  SAFEALLOC(tk,Token)
  tk->code=code;
  printf("%d ", code);
  //tk->line=line;
  tk->next=NULL;
  if(lastToken){
    lastToken->next=tk;
  }else{
    tokens=tk;
  }
  lastToken=tk;
  return tk;
}

Token *addTk2(int code,char *value)
{
  Token *tk;
  SAFEALLOC(tk,Token)
  tk->code=code;
  if(code == ID || code == CT_STRING){
    tk->text = (char*)malloc(strlen(value) * sizeof(char));
    strcpy(tk->text, value);
    printf("%s ",tk->text);
  }
  if(code == CT_INT){
    tk-> integer = atoi(value);//(char*)malloc(strlen(value) * sizeof(char));
    //strcpy(tk->integer, atoi(value));
    printf("%d ",tk->integer);
  }
  if(code == CT_REAL){
    tk->real = atof(value); //(char*)malloc(strlen(value) * sizeof(char));
    //strcpy(tk->real, value);
    printf("%f ", tk->real);
  }
  if(code == CT_CHAR){
    tk->character = value[0];//(char*)malloc(strlen(value) * sizeof(char));
    //strcpy(tk->character, value);
    printf("%c ",tk->character);
  }
  //tk->line=line;
  tk->next=NULL;
  if(lastToken){
    lastToken->next=tk;
  }else{
    tokens=tk;
  }
  lastToken=tk;
  return tk;
}

void tkerr(const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  //fprintf(stderr, "error in line %d: ",tk->line); //optional
  vfprintf(stderr, fmt, va);
  fputc('\n', stderr);
  va_end(va);
  exit(-1);
}

void showTokens();

int getNextToken()
{
  int state=0;
  char ch = fgetc(fp);
  char *token_name = (char *)malloc(256);
  int currentIndex = 0; //dimension of our "rule"
  int flag = 0;
  if(ch == EOF){
    showTokens();
    exit(1);
  }

  //Token *tk;
  while(1){ // infinite loop
    switch(state){
      case 0: // transitions test for state 0
        if(isalpha(ch) || ch == '_'){
          strncpy(token_name, &ch, 1);
          currentIndex++;
          state = 1; //ID
        }
        else if(isspace(ch)){ //consume the spaces
          int un_c = ch;
          while(isspace(un_c)){
            un_c = fgetc(fp);
          }
          ungetc(un_c,fp);
          //pCrtCh++; // consume the character and remains in state 0
        }
        else if(ch == '\0' || ch == -1){
          addTk(END);
          return END;
        }
        // O P E R A T O R S
        else if(ch=='+'){
          addTk(ADD);
          return ADD;
        }
        else if(ch=='='){
          ch = fgetc(fp);
          if (ch == '='){ // we found == EQUAL
            addTk(EQUAL);
            return EQUAL;
          }else{ // we found = ASSIGN
            ungetc(ch, fp);
            addTk(ASSIGN);
            return ASSIGN;
          }
        }
        else if(ch == '-'){
          addTk(SUB);
          return SUB;
        }
        else if(ch == '*'){
          addTk(MUL);
          return MUL;
        }
        else if(ch == '/'){
          ch = fgetc(fp);
          if(ch != '*' && ch != '/'){
            ungetc(ch,fp);
            addTk(DIV);
            return DIV;
          }else if(ch == '*') state = 2;
          else if(ch == '/') state = 3;
        }
        else if(ch == '.'){
          addTk(DOT);
          return DOT;
        }
        else if(ch == '&'){
          ch = fgetc(fp);
          if(ch == '&'){
            addTk(AND);
            return AND;
          }
          ungetc(ch,fp);
          tkerr("Character expected: \'&\'");
        }
        else if(ch == '|'){
          ch = fgetc(fp);
          if(ch == '|'){
            addTk(OR);
            return OR;
          }
          ungetc(ch,fp);
          tkerr("Character expected: \'|\'");
        }
        else if(ch == '>'){
          ch = fgetc(fp);
          if(ch == '='){
            addTk(GREATEREQ);
            return GREATEREQ;
          }
          ungetc(ch,fp);
          addTk(GREATER);
          return GREATER;
        }
        else if(ch == '<'){
          ch = fgetc(fp);
          if(ch == '='){
            addTk(LESSEQ);
            return LESSEQ;
          }
          ungetc(ch,fp);
          addTk(LESS);
          return LESS;
        }
        else if(ch == '!'){
          ch = fgetc(fp);
          if(ch == '='){
            addTk(NOTEQ);
            return NOTEQ;
          }
          ungetc(ch,fp);
          addTk(NOT);
          return NOT;
        }
        // END OF OPERATORS
        // ===================
        // D E L I M I T E R S
        else if(ch == ','){
          addTk(COMMA);
          return COMMA;
        }
        else if(ch == ';'){
          addTk(SEMICOLON);
          return SEMICOLON;
        }
        else if(ch == '('){
          addTk(LPAR);
          return LPAR;
        }
        else if(ch == ')'){
          addTk(RPAR);
          return RPAR;
        }
        else if(ch == '['){
          addTk(LBRACKET);
          return LBRACKET;
        }
        else if(ch == ']'){
          addTk(RBRACKET);
          return RBRACKET;
        }
        else if(ch == '{'){
          addTk(LACC);
          return LACC;
        }
        else if(ch == '}'){
          addTk(RACC);
          return RACC;
        }
        //END OF DELIMITERS
        // ===================
        // C O N S T A N T S
        else if(isdigit(ch)){
          strcpy(token_name, &ch);
          currentIndex++;

          if(ch != '0')
            state = 5;
          else if(ch == '0'){
            ch = fgetc(fp);
            strcpy(token_name + currentIndex, &ch);
            currentIndex++;

            if(ch >= '0' && ch <= '7')
             state = 6; // OCTAL NR ?
            else if(ch == 'x' || ch == 'X')
              state = 7; // HEX NR ?
            //maybe delete this == '.'
            else if(ch == '.'){
              strcpy(token_name + currentIndex, &ch);
              currentIndex++;
              state = 8; // REAL CASE
            }
            else{ // end of number
              if(!isdigit(ch)){ //no longer CT_INT
                token_name[currentIndex] = '\0'; //form CT_INT
                ungetc(ch,fp);
                addTk2(CT_INT,token_name);
                return CT_INT;
              }
              else{ // no other combination
                tkerr("Not int number");
              }
            }
          }
        }
        else if(ch == '\"'){ // CT_STRING
          state = 11;
        }
        else if(ch == '\''){ // CT_CHAR
          state=9;
        }
        break;
      case 1: // IMPLEMENT
        if(isalnum(ch) || ch == '_'){
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;
        }else{
          ungetc(ch,fp);
          state = 10; // KEYWORD
        }
        break;
      case 2: //2
        if(ch != '*') state = 2; //we still have to consume
        else state = 4; //we are almost done
        break;
      case 4: //4
        if(ch != '*' && ch != '/') state = 2; //we are not done consuming comments
        else if(ch == '*') state = 4;
        else if(ch == '/') state = 0; // done
        break;
      case 3:
        if(ch != '\n' && ch != '\r' && ch != '\0' && ch != EOF) state = 3;
        else{
          ungetc(ch,fp);
          state = 0;
        }
        break;
      case 5: // DECIMAL + REAL CASES
        if(isdigit(ch)){
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;
        }
        else if(ch == '.'){
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;
          state = 8; // REAL CASE
        }
        else{
          token_name[currentIndex] = '\0'; //formed a CT_INT
          ungetc(ch,fp);
          addTk2(CT_INT,token_name);
          return CT_INT;
        }
        break;
      case 6: // OCTAL
        if(ch >= '0' && ch <= '7'){ //keep constructing
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;
        }else{ //no other case than 0 -> 7 => END
          token_name[currentIndex] = '\0';
          ungetc(ch,fp);
          addTk2(CT_INT,token_name);
          return CT_INT;
        }
        break;
      case 7: // HEX
        if(isdigit(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;
        }
        else {
          token_name[currentIndex] = '\0';
          ungetc(ch,fp);
          addTk2(CT_INT,token_name);
          return CT_INT;
        }
        break;
      case 8: // REAL
        if(isdigit(ch)){
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;
        }
        else if(ch == 'e' || ch == 'E'){
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;

          ch = fgetc(fp);
          if(ch == '-' || ch == '+'){
            int ch2 = fgetc(fp);
            while(isdigit(ch2)){
              strcpy(token_name + currentIndex, &ch);
              currentIndex++;
              ch2 = fgetc(fp);
            }
            ungetc(ch2,fp);
            token_name[currentIndex] = '\0';
            addTk2(CT_REAL, token_name);
            return CT_REAL;
          }
          else
            tkerr("Not valid exponent");
        }else{
          ungetc(ch,fp);
          token_name[currentIndex] = '\0';
          addTk2(CT_REAL, token_name);
          return CT_REAL;
        }
        break;
      case 9:
        if(ch == '\\'){ //ESC
          strcpy(token_name + currentIndex, &ch);
          currentIndex++;
          ch = fgetc(fp);
          if(strchr("abfntrv\'?\"\\0",ch)){
            strcpy(token_name + currentIndex, &ch);
            currentIndex++;
            ch = fgetc(fp);

            if(ch == '\''){
              addTk2(CT_CHAR, &ch);
              return CT_CHAR;
            }
            else tkerr("Invalid character before \'");
          }
          else tkerr("(CT_CHAR) Invalid character for \'");
        } // 1 - LETTER
        else{
          if(ch != '\''){
            strcpy(token_name + currentIndex, &ch);
            currentIndex++;
            ch = fgetc(fp);
            if(ch == '\''){
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
        ungetc(ch,fp);

             if(!strcmp("break",token_name)){ addTk(BREAK); return BREAK; }
        else if(!strcmp("char",token_name)){ addTk(CHAR); return CHAR; }
        else if(!strcmp("int",token_name)){ addTk(INT); return INT; }
        else if(!strcmp("void",token_name)){ addTk(VOID); return VOID; }
        else if(!strcmp("double",token_name)){ addTk(DOUBLE); return DOUBLE; }
        else if(!strcmp("struct",token_name)){ addTk(STRUCT); return STRUCT; }
        else if(!strcmp("while",token_name)){ addTk(WHILE); return WHILE; }
        else if(!strcmp("return",token_name)){ addTk(RETURN); return RETURN; }
        else if(!strcmp("if",token_name)){ addTk(IF); return IF; }
        else if(!strcmp("else",token_name)){ addTk(ELSE); return ELSE; }
        else if(!strcmp("for",token_name)){ addTk(FOR); return FOR; }
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
      }//aici?

      ch = fgetc(fp);
      if(ch == EOF) flag++;
      if(flag > 2){
        addTk(END);
        return END;
      }
  }
}

/*
void printTokens(int code){
  switch(code){
    case 0:{printf(" ID "); break;}
    case 1:{printf (" BREAK "); break;}
    case 2:{printf (" CHAR "); break;}
    case 3:{printf (" DOUBLE "); break;}
  }
}*/

// PARSER
Token *consumedTk;
Token *currentTk = NULL;

int rule_unit();        //DONE
int rule_declStruct();  //DONE
int rule_declVar();     //DONE
int rule_typeBase();    //DONE
int rule_arrayDecl();   //DONE
int rule_typeName();    //DONE
int rule_declFunc();    //DONE but not sure if ok
int rule_funcArg();     //DONE
int rule_stm();         //DONE but needs re-check
int rule_stmCompound(); //DONE
int rule_expr();        //DONE
int rule_exprAssign();  //DONE but needs re-check
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
int rule_exprPostfix(); //DONE but needs check
int rule_exprPostfix_2(); //DONE but needs check
int rule_exprPrimary(); //DONE but needs check

int consume(int code){
  if(currentTk -> code  == code){
    consumedTk = currentTk;
    currentTk  = currentTk -> next;
    return 1; // consumed => return true
  }
  return 0;  // don't consume it, position is unchanged => return false
}

int rule_unit(){
  while(1){
    if(rule_declStruct()) continue;
    if(rule_declFunc())    continue;
    if(rule_declVar())    continue;
    break;
  }
  if(!consume(END)) tkerr("Missing END token");
  return 1;
}

int rule_declStruct(){
  Token *startTk = currentTk;

  if(!consume(STRUCT)) return 0;
  if(!consume(ID)) tkerr("Missing ID after STRUCT declaration.");
  if(!consume(LACC)) { currentTk = startTk; return 0;} //tkerr...
  while(1){
    if(rule_declVar()) continue;
    break;
  }
  if(!consume(RACC)) tkerr("Missing RACC after STRUCT declaration.");
  if(!consume(SEMICOLON)) tkerr("Missing SEMICOLON after STRUCT declaration.");

  return 1;
}

int rule_declVar(){ //need to re-look here
  Token *startTk = currentTk;

  if(rule_typeBase()){
    if(consume(ID)){
      rule_arrayDecl();
      while(1){
        if(consume(COMMA)){
          if(consume(ID)){
            rule_arrayDecl();
            continue;
          }else tkerr("Missing ID after COMMA in VAR declaration.");
        }
        break;
      }
      if(consume(SEMICOLON)) return 1;
      else tkerr("Missing SEMICOLON after VAR declaration.");
    }
    else currentTk = startTk;
  } // while is done
  else currentTk = startTk;

  return 0;
}

int rule_typeBase(){
  Token *startTk = currentTk;

  if(consume(INT)) return 1;
  if(consume(DOUBLE)) return 1;
  if(consume(CHAR)) return 1;
  if(consume(STRUCT)){
    if(consume(ID)){
      return 1;
    }
    else tkerr("Missing ID after STRUCT.");
  }
  currentTk = startTk;

  return 0;
}

int rule_arrayDecl(){
  Token *startTk = currentTk;
  if(consume(LBRACKET)){
    rule_expr();
    if(consume(RBRACKET)){
      return 1;
    }
    else tkerr("Missing RBRACKET after array declaration.");
  }
  currentTk = startTk;

  return 0;
}

int rule_typeName(){
  if(rule_typeBase()){
    rule_arrayDecl();
    return 1;
  }

  return 0;
}

int rule_declFunc_helper(){
  Token *startTk = currentTk;

  if(consume(ID)){
    if(consume(LPAR)){
      if(rule_funcArg()){
        while(1){
          if(consume(COMMA)){
            if(rule_funcArg()){
              //return 1;
              continue;
            }else tkerr("Missing FUNC_ARG for function declaration.");
          }else break;
        }
      }
      if(consume(RPAR)){
        if(rule_stmCompound()){
          return 1;
        }else tkerr("Missing stmCompound for function declaration.");
      }else tkerr("Missing RPAR for function declaration.");
    }
  }
  currentTk = startTk;

  return 0;
}

int rule_declFunc(){
  Token *startTk = currentTk;

  if(rule_typeBase()){
    consume(MUL);
    if(!rule_declFunc_helper()){
      currentTk = startTk; return 0;
    }
    return 1;
  }
  if(consume(VOID)){
    if(!rule_declFunc_helper())
      tkerr("Missing ID after VOID declaration.");
    return 1;
  }

  return 0;
}

int rule_funcArg(){
  Token *startTk = currentTk;

  if(rule_typeBase()){
    if(consume(ID)){
      rule_arrayDecl();
      return 1;
    }else tkerr("Missing ID for function argument.");
  }
  currentTk = startTk;

  return 0;
}

int rule_stm(){
  Token *startTk = currentTk;

  if(rule_stmCompound()) return 1;
  //IF LPAR expr RPAR stm (ELSE stm)?
  if(consume(IF)){
    if(consume(LPAR)){
      if(rule_expr()){
        if(consume(RPAR)){
          if(rule_stm()){
            if(consume(ELSE)){
              if(rule_stm()) return 1;
              else tkerr("Missing STM after else.");
            }
            return 1;
          }else tkerr("Missing STM declaration.");
        }else tkerr("Missing RPAR declaration.");
      }else tkerr("Missing EXPR declaration.");
    }else tkerr("Missing LPAR declaration.");
  }
  currentTk = startTk; //not sure if needed

  //WHILE LPAR expr RPAR stm
  if(consume(WHILE)){
    if(consume(LPAR)){
      if(rule_expr()){
        if(consume(RPAR)){
          if(rule_stm()){
            return 1;
          }else tkerr("Missing STM declaration.");
        }else tkerr("Missing RPAR declaration.");
      }else tkerr("Missing EXPR declaration.");
    }else tkerr("Missing LPAR declaration.");
  }
  currentTk = startTk;

  //FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
  if(consume(FOR)){
    if(!consume(LPAR)) tkerr("Missing LPAR declaration.");
    rule_expr();
    if(!consume(SEMICOLON)) tkerr("Missing SEMICOLON declaration.");
    rule_expr();
    if(!consume(SEMICOLON)) tkerr("Missing SEMICOLON declaration.");
    rule_expr();
    if(!consume(RPAR)) tkerr("Missing RPAR declaration.");
    if(!rule_stm()) tkerr("Missing STM declaration.");
    return 1;
  }
  currentTk = startTk;

  //BREAK SEMICOLON
  if(consume(BREAK)){
    if(consume(SEMICOLON))
      return 1;
    else tkerr("Missing SEMICOLON declaration.");
  }
  currentTk = startTk;

  //RETURN expr? SEMICOLON
  if(consume(RETURN)){
    rule_expr();
    if(consume(SEMICOLON)){
      return 1;
    }else tkerr("Missing SEMICOLON declaration.");
  }
  currentTk = startTk;

  //expr? SEMICOLON
  if(consume(SEMICOLON)){
    return 1;
  }else{ //CHANGED HERE
    if(rule_expr()){
      if(!consume(SEMICOLON)) tkerr("Missing SEMICOLON declaration");
      return 1;
    }
  }
  currentTk = startTk;
  return 0;
}

//LACC (declVar | stm)* RACC
int rule_stmCompound(){
    Token *startTk = currentTk;

    if(consume(LACC)){
      while(1){
        if(rule_declVar()) continue;
        if(rule_stm())     continue;
        else break;
      }
      if(consume(RACC))
        return 1;
      else tkerr("Missing RACC declaration.");
    }
    currentTk = startTk;

    return 0;
}

int rule_expr(){
  Token *startTk = currentTk;

  if(rule_exprAssign())
    return 1;

  return 0;
}

//exprUnary ASSIGN exprAssign | exprOR
int rule_exprAssign(){
  Token *startTk = currentTk;

  if(rule_exprUnary()){
    if(consume(ASSIGN)){
      if(rule_exprAssign())
        return 1;
      else tkerr("Missing exprAssign declaration.");
    }else tkerr("Missing ASSIGN declaration.");
  }
  if(rule_exprOr()) return 1;
  currentTk = startTk;
  return 0;
}

int rule_exprOr_2(){
  Token *startTk = currentTk;

  if(consume(OR)){
    if(rule_exprAnd()){
      rule_exprOr_2();
        return 1;
    }else tkerr("Missing expression after OR.");
  }
  currentTk = startTk;

  return 1;
}

int rule_exprOr(){
  Token *startTk = currentTk;

  if(rule_exprAnd()){
    if(rule_exprOr_2())
      return 1;
    else tkerr("Incomplete OR expression.");
  }

  return 0;
}

int rule_exprAnd_2(){
  Token *startTk = currentTk;

  if(consume(AND)){
    if(rule_exprEq()){
      rule_exprAnd_2();
      return 1;
    }else tkerr("Missing expression after AND.");
  }
  currentTk = startTk;

  return 1;
}

int rule_exprAnd(){
  Token *startTk = currentTk;

  if(rule_exprEq()){
    if(rule_exprAnd_2())
    return 1;
    else tkerr("Incomplete EQ expression.");
  }

  return 0;
}

int rule_exprEq_2(){
  Token *startTk = currentTk;

  if(consume(EQUAL)){
    if(rule_exprRel()){
      rule_exprEq_2();
      return 1;
    }else tkerr("Missing REL expression.");
  }

  if(consume(NOTEQ)){
    if(rule_exprRel()){
      rule_exprEq_2();
      return 1;
    }else tkerr("Missing REL expression.");
  }
  currentTk = startTk;

  return 1;
}

int rule_exprEq(){
  Token *startTk = currentTk;

  if(rule_exprRel()){
    if(rule_exprEq_2())
    return 1;
    else tkerr("Incomplete EQ expression.");
  }

  return 0;
}

int rule_exprRel_3(){
  if(consume(LESS))      return 1;
  if(consume(LESSEQ))    return 1;
  if(consume(GREATER))   return 1;
  if(consume(GREATEREQ)) return 1;
  return 0;
}

int rule_exprRel_2(){
  Token *startTk = currentTk;

  if(rule_exprRel_3()){
    if(rule_exprAdd()){
      rule_exprRel_2();
      return 1;
    }else tkerr("Missing ADD expression.");
  }
  currentTk = startTk;

  return 1;
}

int rule_exprRel(){
  Token *startTk = currentTk;

  if(!rule_exprAdd()) return 0;
  if(!rule_exprRel_2()) tkerr("Incomplete REL expression.");
  currentTk = startTk;

  return 0;
}

int rule_exprAdd_2(){
  Token *startTk = currentTk;

  if(consume(ADD)){
    if(rule_exprAdd()){
      rule_exprAdd_2();
      return 1;
    }else tkerr("Missing REL expression.");
  }

  if(consume(SUB)){
    if(rule_exprAdd()){
      rule_exprAdd_2();
      return 1;
    }else tkerr("Missing REL expression.");
  }
  currentTk = startTk;

  return 1;
}

int rule_exprAdd(){
  Token *startTk = currentTk;

  if(rule_exprMul()){
    if(rule_exprAdd_2())
    return 1;
    else tkerr("Incomplete ADD expression.");
  }

  return 0;
}

int rule_exprMul_2(){
  Token *startTk = currentTk;

  if(consume(MUL)){
    if(rule_exprCast()){
      rule_exprMul_2();
      return 1;
    }else tkerr("Missing REL expression.");
  }

  if(consume(DIV)){
    if(rule_exprCast()){
      rule_exprMul_2();
      return 1;
    }else tkerr("Missing REL expression.");
  }
  currentTk = startTk;

  return 1;
}

int rule_exprMul(){
  Token *startTk = currentTk;

  if(rule_exprCast()){
    if(rule_exprMul_2())
    return 1;
    else tkerr("Incomplete MUL expression.");
  }

  return 0;
}

//LPAR typeName RPAR exprCast | exprUnary
int rule_exprCast(){
  Token *startTk = currentTk;

  if(consume(LPAR)){
    if(rule_typeName()){
      if(consume(RPAR)){
        if(rule_exprCast){
          return 1;
        }else tkerr("Incomplete CAST declaration.");
      }else tkerr("Missing RPAR expression after cast declaration.");
    }else tkerr("Missing TYPENAME expression after LPAR declaration.");
  }
  if(rule_exprUnary()){
    return 1;
  }
  currentTk = startTk;

  return 0;
}

// (SUB | NOT) exprUnary | exprPostfix
int rule_exprUnary(){
  Token *startTk = currentTk;

  if(consume(SUB)){
    if(rule_exprUnary())
      return 1;
    else tkerr("Missing UNARY expression.");
  }

  if(consume(NOT)){
    if(rule_exprUnary())
      return 1;
    else tkerr("Missing UNARY expression.");
  }

  if(rule_exprPostfix()){
    return 1;
  }
  currentTk = startTk;

  return 0;
}

int rule_exprPostfix_2(){
  Token *startTk = currentTk;
  if(consume(LBRACKET)){
    if(rule_expr()){
      if(consume(RBRACKET)){
        if(rule_exprPostfix_2()) return 1;
        else tkerr(" Incomplete EXPRESSION declaration");
      }else tkerr(" Missing RBACKET after expression");
    }else tkerr(" Missing EXPRESSION after LBRACKET");
  }
  if(consume(DOT)){
    if(consume(ID)){
      if(rule_exprPostfix_2()) return 1;
      else tkerr(" Incomplete EXPRESSION declaration");
    }else tkerr("Missing ID in postfix expression");
  }
  currentTk = startTk;
  return 1;
}

int rule_exprPostfix(){
  Token *startTk = currentTk;
  if(rule_exprPrimary()){
    if(rule_exprPostfix_2()){
      return 1;
    }else tkerr("Incomplete POSTFIX declaration");
  }
  currentTk = startTk;
  return 0;
}

int rule_exprPrimary(){
  Token *startTk = currentTk;
  if(consume(ID)){
    if(consume(LPAR)){
      if(rule_expr()){
        while(1){
          if(consume(COMMA)){
            if(rule_expr()) continue;
            else tkerr("Missing expression after ','");
          }
          break;
        }
      }
      if(consume(RPAR)){
        return 1;
      }else tkerr("Missing RPAR in primary EXPR");
    }
    return 1;
  }
  if(consume(CT_INT)) return 1;

  if(consume(CT_REAL)) return 1;

  if(consume(CT_CHAR)) return 1;

  if(consume(CT_STRING)) return 1;

  if(consume(LPAR)){
    if(rule_expr()){
      if(consume(RPAR))return 1;
      else tkerr("Missing RPAR in primary EXPR");
    }else tkerr("Missing expression after LPAR");
  }

  currentTk = startTk;
  return 0;
}


void start_lexor(Token *startTk){
  currentTk = startTk;
  if(rule_unit()) printf("ASYN DONE");
  else tkerr("Error at ASYN");
}

// END OF PARSER
void showTokens() {
    Token* p = tokens;
    while(p) {
        printf("(%d",p->code);
        if(p->code == ID || p->code == CT_CHAR || p->code == CT_STRING) printf(":%s)",p->text);
        else if(p->code == CT_INT) printf(":%ld)",p->integer);
        else if(p->code == CT_REAL) printf(":%lf)",p->real);
        printf(" ");
        p = p->next;
    }
    printf("\n");
}

int main(int argc, char ** argv){
  if (argc < 2){
    printf("Wrong nr. of params.");
    exit(1);
  }
  printf("Compiling file: %s\n",argv[1]);

  fp = fopen(argv[1],"r");
  if(!fp) err("Cannot open the source file");

  int c;
  tokens = (Token *)malloc(1000*sizeof(Token));
  /*
  while((c = fgetc(fp)) != EOF){
    if(c == EOF)
    break;
    ungetc(c,fp);
    printf("%d ",getNextToken(fp));
    //printf("HERE");
    //voiam sa fac printToken(getNextToken(fp))
    //getNextToken(fp,c);
  }*/
  while(getNextToken(fp) != END);
  printf("\nALEX IS DONE\n");
  showTokens();
  printf("\n");
  Token* p = tokens;
  start_lexor(p);
  return 0;
}
