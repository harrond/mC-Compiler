#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "MiniC.tbl"
#define PS_SIZE 200
#define FNAMESIZE 12

#define NO_KEYWORDS 7
#define ID_LENGTH 12

struct tokenType {
	int number; //토큰 번호
	union {
		char id[ID_LENGTH];
		int num;
	}value;
	//char *tokenValue; //토큰 값
};

typedef struct nodeType {
	struct tokenType token;	//토큰 종류
	enum { terminal, nonterm } noderep;	//노드 종류
	struct nodeType *son;	//왼쪽 링크
	struct nodeType *brother; //오른쪽 링크
}Node;

int errcnt = 0;
int sp;
int stateStack[PS_SIZE];
int symbolStack[PS_SIZE];
int valueStack[PS_SIZE];

FILE *fp;
FILE *astFile;
char filename[FNAMESIZE];
char outputFile[FNAMESIZE];



enum tsymbol {
	tnull = -1,
	tnot, tnotequ, tmod, tmodAssign, tident, tnumber,
	tand, tlparen, trparen, tmul, tmulAssign, tplus,
	tinc, taddAssign, tcomma, tminus, tdec, tsubAssign,
	tdiv, tdivAssign, tsemicolon, tless, tlesse, tassign,
	tequal, tgreat, tgreate, tlbracket, trbracket, teof,
	/************* word symbols ***************/
	tconst, telse, tif, tint, treturn, tvoid, twhile, tlbrace, tor, trbrace
};

enum nodeNumber {
	ACTUAL_PARAM, ADD, ADD_ASSIGN, ARRAY_VAR, ASSIGN_OP,
	CALL, COMPOUND_ST, CONST_NODE, DCL, DCL_ITEM,
	DCL_LIST, DCL_SPEC, DIV, DIV_ASSIGN, EQ,
	ERROR_NODE, EXP_ST, FORMAL_PARA, FUNC_DEF, FUNC_HEAD,
	GE, GT, IDENT, IF_ELSE_ST, IF_ST,
	INDEX, INT_NODE, LE, LOGICAL_AND, LOGICAL_NOT,
	LOGICAL_OR, LT, MOD, MOD_ASSIGN, MUL,
	MUL_ASSIGN, NE, NUMBER, PARAM_DCL, POST_DEC,
	POST_INC, PRE_DEC, PRE_INC, PROGRAM, RETURN_ST,
	SIMPLE_VAR, STAT_LIST, SUB, SUB_ASSIGN, UNARY_MINUS,
	VOID_NODE, WHILE_ST
};
char *nodeName[] = {
	"ACTUAL_PARAM", "ADD", "ADD_ASSIGN", "ARRAY_VAR", "ASSIGN_OP",
	"CALL", "COMPOUND_ST", "CONST_NODE", "DCL", "DCL_ITEM",
	"DCL_LIST", "DCL_SPEC", "DIV", "DIV_ASSIGN", "EQ",
	"ERROR_NODE", "EXP_ST", "FORMAL_PARA", "FUNC_DEF", "FUNC_HEAD",
	"GE", "GT", "IDENT", "IF_ELSE_ST", "IF_ST",
	"INDEX", "INT_NODE", "LE", "LOGICAL_AND", "LOGICAL_NOT",
	"LOGICAL_OR", "LT", "MOD", "MOD_ASSIGN", "MUL",
	"MUL_ASSIGN", "NE", "NUMBER", "PARAM_DCL", "POST_DEC",
	"POST_INC", "PRE_DEC", "PRE_INC", "PROGRAM", "RETURN_ST",
	"SIMPLE_VAR", "STAT_LIST", "SUB", "SUB_ASSIGN", "UNARY_MINUS",
	"VOID_NODE", "WHILE_ST"
};
int ruleName[] = {
	0,PROGRAM,0,0,0,
	0,FUNC_DEF,FUNC_HEAD,DCL_SPEC,0,
	0,0,0,CONST_NODE,INT_NODE,
	VOID_NODE,0,FORMAL_PARA,0,0,
	0,0,PARAM_DCL,COMPOUND_ST,DCL_LIST,
	DCL_LIST,0,0,DCL,0,
	0,DCL_ITEM,DCL_ITEM,SIMPLE_VAR,ARRAY_VAR,
	0,0,STAT_LIST,0,0,
	0,0,0,0,0,
	0,EXP_ST,0,0,IF_ST,
	IF_ELSE_ST,WHILE_ST,RETURN_ST,0,0,
	ASSIGN_OP,ADD_ASSIGN,SUB_ASSIGN,MUL_ASSIGN,DIV_ASSIGN,
	MOD_ASSIGN,0,LOGICAL_OR,0,LOGICAL_AND,
	0,EQ,NE,0,GT,
	LT,GE,LE,0,ADD,
	SUB,0,MUL,DIV,MOD,
	0,UNARY_MINUS,LOGICAL_NOT,PRE_INC,PRE_DEC,
	0,INDEX,CALL,POST_INC,POST_DEC,
	0,0,ACTUAL_PARAM,0,0,
	0,0,0
};

char *keyword[NO_KEYWORDS] = {
	"const", "else", "if", "int", "return", "void", "while"
};

enum tsymbole tnum[NO_KEYWORDS] = {
	tconst, telse, tif, tint, treturn, tvoid, twhile
};

char *tokenName[] = {

	"!", "!=", "%", "%=", "%ident", "%number",
	"&&", "(", ")", "*", "*=", "+", "++", "+=",
	",", "-", "--", "-=", "/", "/=", ";", "<",
	"<=", "=", "==", ">", ">=", "[", "]", "eof",
	//************word symbols****************//
	"const", "else", "if", "int", "return", "void",
	"while", "{", "||", "}"
};


void lexicalError(int n) {
	printf(" *** Lexical Error : ");
	switch (n)
	{
	case 1: printf("an identifier length must be less than 12.\n");
		break;
	case 2: printf("next character must be &.\n");
		break;
	case 3: printf("next character must be |.\n");
		break;
	case 4: printf("invalid character!!!\n");
		break;
	}
}
int superLetter(char ch) {
	if (isalpha(ch) || ch == '_') return 1;
	else return 0;
}
int superLetterOrDigit(char ch) {
	if (isalnum(ch) || ch == '_') return 1;
	else return 0;
}
int hexValue(char ch) {
	switch (ch) {
	case '0': case '1': case '2': case '3': case '4': case '5': case '6':
	case '7': case '8': case '9': return (ch - '0');
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return (ch - 'A' + 10);
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return (ch - 'a' + 10);
	default:
		return -1;
	}
}

int getIntNum(char firstCharacter) {
	int num = 0;
	int value;
	char ch;

	if (firstCharacter != '0') {
		ch = firstCharacter;
		do {
			num = 10 * num + (int)(ch - '0');
			ch = getc(fp);
		} while (isdigit(ch));
	}
	else {
		ch = getc(fp);
		if ((ch >= '0') && (ch <= '7'))
			do {
				num = 8 * num + (int)(ch - '0');
				ch = getc(fp);
			} while ((ch >= '0') && (ch <= '7'));
		else if ((ch == 'X') || (ch == 'x')) {
			while ((value = hexValue(ch = getc(fp))) != -1)
				num = 16 * num + value;
		}
		else num = 0;
	}
	ungetc(ch, fp);
	return num;
}

struct tokenType scanner() {
	struct tokenType token;
	int i, index;
	char ch, id[ID_LENGTH];

	token.number = tnull;
	do {

		while (isspace(ch = getc(fp)));

		if (superLetter(ch)) {
			i = 0;
			do {
				if (i < ID_LENGTH)id[i++] = ch;
				ch = getc(fp);
			} while (superLetterOrDigit(ch));
			if (i >= ID_LENGTH) lexicalError(1);
			id[i] = '\0';
			ungetc(ch, fp);//retract
						   //find the identifier in the keyword table
			for (index = 0; index < NO_KEYWORDS; index++)
				if (!strcmp(id, keyword[index]))break;
			if (index < NO_KEYWORDS)//found,keyword exit
				token.number = tnum[index];
			else {
				token.number = tident;
				strcpy(token.value.id, id);
			}
		}//end of identifier or keyword
		else if (isdigit(ch)) {//integer constant
			token.number = tnumber;
			token.value.num = getIntNum(ch);
		}
		else switch (ch) { //special character
		case '/': //state 10
			ch = getc(fp);
			if (ch == '*') //text comment
				do {
					while (ch != '*') ch = getc(fp);
					ch = getc(fp);
				} while (ch != '/');
			else if (ch == '/') //line comment
				while (getc(fp) != '\n');
			else if (ch == '=') token.number = tdivAssign;
			else {
				token.number = tdiv;
				ungetc(ch, fp); //retract
			}
			break;
		case '!': //state 17
			ch = getc(fp);
			if (ch == '=') token.number = tnotequ;
			else {
				token.number = tnot;
				ungetc(ch, fp); //retract
			}
			break;
		case '%': //state 20
			ch = getc(fp);
			if (ch == '=') token.number = tmodAssign;
			else {
				token.number = tmod;
				ungetc(ch, fp); //retract
			}
			break;
		case '&': //state 23
			ch = getc(fp);
			if (ch == '&') token.number = tand;
			else {
				lexicalError(2);
				ungetc(ch, fp); //retract
			}
			break;
		case '*': //state 25
			ch = getc(fp);
			if (ch == '=') token.number = tmulAssign;
			else {
				token.number = tmul;
				ungetc(ch, fp); //retract
			}
			break;
		case '+': //state 28
			ch = getc(fp);
			if (ch == '+') token.number = tinc;
			else if (ch == '=') token.number = taddAssign;
			else {
				token.number = tplus;
				ungetc(ch, fp); //retract
			}
			break;
		case '-': //state 32
			ch = getc(fp);
			if (ch == '-') token.number = tdec;
			else if (ch == '=') token.number = tsubAssign;
			else {
				token.number = tminus;
				ungetc(ch, fp); //retract
			}
			break;
		case '<': //state 36
			ch = getc(fp);
			if (ch == '=') token.number = tlesse;
			else {
				token.number = tless;
				ungetc(ch, fp); //retract
			}
			break;
		case '=': //state 39
			ch = getc(fp);
			if (ch == '=') token.number = tequal;
			else {
				token.number = tassign;
				ungetc(ch, fp); //retract
			}
			break;
		case '>': //state 42
			ch = getc(fp);
			if (ch == '=') token.number = tgreate;
			else {
				token.number = tgreat;
				ungetc(ch, fp); //retract
			}
			break;
		case '|': //state 23
			ch = getc(fp);
			if (ch == '|') token.number = tor;
			else {
				lexicalError(3);
				ungetc(ch, fp); //retract
			}
			break;
		case '(': token.number = tlparen; break;
		case ')': token.number = trparen; break;
		case ',': token.number = tcomma; break;
		case ';': token.number = tsemicolon; break;
		case '[': token.number = tlbracket; break;
		case ']': token.number = trbracket; break;
		case '{': token.number = tlbrace; break;
		case '}': token.number = trbrace; break;
		case EOF: token.number = teof; break;
		default: {
			printf("Current character : %c", ch);
			lexicalError(4);
			break;
		}
		}//switch end

	} while (token.number == tnull);

	return token;
}//end of scanner

int meaningfulToken(struct tokenType token) {
	if ((token.number == tident) || (token.number == tnumber)) return 1;
	else return 0;
}

Node *buildNode(struct tokenType token) {
	Node *ptr;
	ptr = (Node *)malloc(sizeof(Node));
	if (!ptr) {
		printf("malloc error in buildNode()\n");
		exit(1);
	}
	ptr->token = token;
	ptr->noderep = terminal;
	ptr->son = ptr->brother = NULL;
	return ptr;
}

Node *buildTree(int nodeNumber, int rhsLength) {
	int i, j, start;
	Node *first, *ptr;

	i = sp - rhsLength + 1;
	//step1: find a first index with node in value stack
	while (i <= sp&&valueStack[i] == NULL) i++;
	if (!nodeNumber&&i > sp) return NULL;
	start = i;
	//step2 : linking brothers
	while (i <= sp - 1) {
		j = i + 1;
		while (j <= sp&&valueStack[j] == NULL) j++;
		if (j <= sp) {
			ptr = valueStack[i];
			while (ptr->brother) ptr = ptr->brother;
			ptr->brother = valueStack[j];
		}
		i = j;
	}
	first = (start > sp) ? NULL : valueStack[start];
	//step3 : making subtree root and linking son
	if (nodeNumber) {
		ptr = (Node *)malloc(sizeof(Node));
		if (!ptr) {
			printf("malloc error in buildNode()\n");
			exit(1);
		}
		ptr->token.number = nodeNumber;
		//ptr->token.value.id = NULL;
		ptr->noderep = nonterm;
		ptr->son = first;
		ptr->brother = NULL;
		return ptr;
	}
	else return first;

}

void printNode(Node *pt, int indent) {

	int i;

	for (i = 1; i <= indent; i++) fprintf(astFile, " ");
	if (pt->noderep == terminal) {
		if (pt->token.number == tident) {
			printf("%s", pt->token.value.id);
			fprintf(astFile, " Terminal: %s", pt->token.value.id);

		}
		else if (pt->token.number == tnumber) {
			printf("%d", pt->token.value.num);
			fprintf(astFile, " Terminal: %d", pt->token.value.num);

		}
	}
	else {//nonterminal node
		int i;
		i = (int)(pt->token.number);
		fprintf(astFile, " Nonterminal: %s", nodeName[i]);

	}
	fprintf(astFile, "\n");
}

void printTree(Node *pt, int indent) {
	Node *p = pt;
	while (p != NULL) {
		printf("1\n");
		printNode(p, indent);
		printf("2\n");
		if (p->noderep == nonterm) printTree(p->son, indent + 5);
		p = p->brother;
	}
}


void printToken(struct tokenType token) {
	if (token.number == tident)
		printf("%s", token.value.id);
	else if (token.number == tnumber)
		printf("%d", token.value.id);
	else
		printf("%s", tokenName[token.number]);
}

void dumpStack() {
	int i, start;
	if (sp > 10) start = sp - 10;
	else start = 0;
	printf("\n*** dump state stack : ");
	for (i = start; i <= sp; ++i) {
		printf(" %d", stateStack[i]);
	}
	printf("\n*** dump symbol stack : ");
	for (i = start; i <= sp; ++i) {
		printf(" %d", symbolStack[i]);
	}
	printf("\n");
}

void errorRecovery() {
	struct tokenType tok;
	int parenthesisCount, braceCount;
	int i;
	//step 1 : skip to the semicolon
	parenthesisCount = braceCount = 0;
	while (1) {
		tok = scanner();
		if (tok.number == teof) exit(1);
		if (tok.number == tlparen)parenthesisCount++;
		else if (tok.number == trparen)parenthesisCount--;
		if (tok.number == tlbrace) braceCount++;
		else if (tok.number == trbrace) braceCount--;
		if ((tok.number == tsemicolon) && (parenthesisCount <= 0) && (braceCount <= 0))
			break;
	}
	//step 2 : adjust state stack
	for (i = sp; i >= 0; i--) {
		if (stateStack[i] == 36)break;//second statement part
		if (stateStack[i] == 24) break; //first statement part
		if (stateStack[i] == 25) break; //second internal dcl
		if (stateStack[i] == 17) break; //internal declaration
										//external declaration
		if (stateStack[i] == 2) break;//after first external dcl
		if (stateStack[i] == 0) break;//first external declaration
	}
	sp = i;
}

Node *Parser() {
	extern int parsingTable[NO_STATES][NO_SYMBOLS + 1];
	extern int leftSymbol[NO_RULES + 1], rightLength[NO_RULES + 1];
	int entry, ruleNumber, lhs;
	int currentState;
	struct tokenType token;
	Node *ptr;

	sp = 0; stateStack[sp] = 0; //초기치
	token = scanner();

	while (1) {
		currentState = stateStack[sp];
		entry = parsingTable[currentState][token.number];
		if (entry > 0) { /* shift action */

			if (++sp > PS_SIZE) {
				printf("critical compiler error: parsing stack overflow");
				exit(1);
			}
			symbolStack[sp] = token.number;
			stateStack[sp] = entry;
			valueStack[sp] = meaningfulToken(token) ? buildNode(token) : NULL;
			token = scanner();
		}
		else if (entry < 0) {
			ruleNumber = -entry;
			if (ruleNumber == GOAL_RULE) { /* Accept action */
				printf("*** valid source ***\n");
				return valueStack[sp - 1];
			}
			ptr = buildTree(ruleName[ruleNumber], rightLength[ruleNumber]);
			sp = sp - rightLength[ruleNumber];
			lhs = leftSymbol[ruleNumber];
			currentState = parsingTable[stateStack[sp]][lhs];

			symbolStack[++sp] = lhs;
			stateStack[sp] = currentState;
			valueStack[sp] = ptr;

		}
		else {		/* error action */
			printf(" === error in source === \n");
			errcnt++;
			printf("Current Token :");
			printToken(token);
			dumpStack();
			errorRecovery();
			token = scanner();
		}
	}

}//parser



void main(int argc, char *argv[]) {
	Node *root;
	if (argc != 3) {
		printf("Argument error!\n");
		exit(1);
	}
	strcpy(filename, argv[1]);
	strcpy(outputFile, argv[2]);

	if ((fp = fopen(filename, "r")) == NULL) {
		printf("Read File open error!\n");
		exit(2);
	}
	if ((astFile = fopen(outputFile, "w+")) == NULL) {
		printf("Output File open error!\n");
		exit(2);
	}

	root = Parser();
	printTree(root, 2);

	fclose(fp);
	fclose(astFile);
}