/**
 * Parser class for a simple interpreter.
 *
 * (c) 2020 by Ronald Mak
 * Department of Computer Science
 * San Jose State University
 */
#include <string>
#include <map>

#include "Token.h"
#include "Parser.h"

namespace frontend {

using namespace std;

set<TokenType> Parser::statementStarters;
set<TokenType> Parser::statementFollowers;
set<TokenType> Parser::relationalOperators;
set<TokenType> Parser::simpleExpressionOperators;
set<TokenType> Parser::termOperators;
set<TokenType> Parser::factorOperators;

void Parser::initialize()
{
    // Tokens that can start a statement.
    statementStarters.insert(BEGIN);
    statementStarters.insert(IDENTIFIER);
    statementStarters.insert(REPEAT);
    statementStarters.insert(WHILE);
    statementStarters.insert(TokenType::WRITE);
    statementStarters.insert(TokenType::WRITELN);

    // Tokens that can immediately follow a statement.
    statementFollowers.insert(SEMICOLON);
    statementFollowers.insert(END);
    statementFollowers.insert(UNTIL);
    statementFollowers.insert(END_OF_FILE);
    statementFollowers.insert(DO);

    relationalOperators.insert(EQUALS);
    relationalOperators.insert(LESS_THAN);
    relationalOperators.insert(LESS_EQUALS);
    relationalOperators.insert(GREATER_THAN);
    relationalOperators.insert(GREATER_EQUALS);
    relationalOperators.insert(NOT_EQUALS);

    simpleExpressionOperators.insert(PLUS);
    simpleExpressionOperators.insert(MINUS);

    termOperators.insert(STAR);
    termOperators.insert(SLASH);

    factorOperators.insert(TokenType::NOT);
}

Node *Parser::parseProgram()
{
    Node *programNode = new Node(NodeType::PROGRAM);

    currentToken = scanner->nextToken();  // first token!

    if (currentToken->type == TokenType::PROGRAM)
    {
        currentToken = scanner->nextToken();  // consume PROGRAM
    }
    else syntaxError("Expecting PROGRAM");

    if (currentToken->type == IDENTIFIER)
    {
        string programName = currentToken->text;
        symtab->enter(programName);
        programNode->text = programName;

        currentToken = scanner->nextToken();  // consume program name
    }
    else syntaxError("Expecting program name");

    if (currentToken->type == SEMICOLON)
    {
        currentToken = scanner->nextToken();  // consume ;
    }
    else syntaxError("Missing ;");

    if (currentToken->type != BEGIN) syntaxError("Expecting BEGIN");

    // The PROGRAM node adopts the COMPOUND tree.
    programNode->adopt(parseCompoundStatement());

    if (currentToken->type == SEMICOLON) syntaxError("Expecting .");
    return programNode;
}

Node *Parser::parseStatement()
{
//	cout << "parse statement : " << currentToken->text << " : " << currentToken->lineNumber << endl;
    Node *stmtNode = nullptr;
    int savedLineNumber = currentToken->lineNumber;
    lineNumber = savedLineNumber;

    switch (currentToken->type)
    {
        case IDENTIFIER : stmtNode = parseAssignmentStatement(); break;
        case BEGIN :      stmtNode = parseCompoundStatement();   break;
        case REPEAT :     stmtNode = parseRepeatStatement();     break;
        case WRITE :      stmtNode = parseWriteStatement();      break;
        case WRITELN :    stmtNode = parseWritelnStatement();    break;
        case WHILE :	  stmtNode = parseWhileStatement(); 	 break;
        case SEMICOLON :  stmtNode = nullptr; break;  // empty statement

        default : syntaxError("Unexpected token");
    }

    if (stmtNode != nullptr) stmtNode->lineNumber = savedLineNumber;
    return stmtNode;
}

Node *Parser::parseAssignmentStatement()
{
//	cout << "parse assignment statement : " << currentToken->text << " : " << currentToken->lineNumber << endl;
    // The current token should now be the left-hand-side variable name.

    Node *assignmentNode = new Node(ASSIGN);

    // Enter the variable name into the symbol table
    // if it isn't already in there.
    string variableName = currentToken->text;
    SymtabEntry *variableId = symtab->lookup(toLowerCase(variableName));
    if (variableId == nullptr) variableId = symtab->enter(variableName);

    // The assignment node adopts the variable node as its first child.
    Node *lhsNode  = new Node(VARIABLE);
    lhsNode->text  = variableName;
    lhsNode->entry = variableId;
    assignmentNode->adopt(lhsNode);

    currentToken = scanner->nextToken();  // consume the LHS variable;

    if (currentToken->type == COLON_EQUALS)
    {
        currentToken = scanner->nextToken();  // consume :=
    }
    else syntaxError("Missing :=");

    // The assignment node adopts the expression node as its second child.
    Node *rhsNode = parseExpression();
    assignmentNode->adopt(rhsNode);

    return assignmentNode;
}

Node *Parser::parseCompoundStatement()
{
//	cout << "parse compound statement" << endl;
    Node *compoundNode = new Node(COMPOUND);
    compoundNode->lineNumber = currentToken->lineNumber;

    currentToken = scanner->nextToken();  // consume BEGIN
    parseStatementList(compoundNode, END);

    if (currentToken->type == END)
    {
        currentToken = scanner->nextToken();  // consume END
//        cout << "should be current token end : " << currentToken->text << endl;
    }
    else syntaxError("Expecting END");

    return compoundNode;
}

void Parser::parseStatementList(Node *parentNode, TokenType terminalType)
{
    while (   (currentToken->type != terminalType)
           && (currentToken->type != END_OF_FILE))
    {
        Node *stmtNode = parseStatement();
        if (stmtNode != nullptr) parentNode->adopt(stmtNode);

        // A semicolon separates statements.
        if (currentToken->type == SEMICOLON)
        {
            while (currentToken->type == SEMICOLON)
            {
                currentToken = scanner->nextToken();  // consume ;
            }
        }
        else if (statementStarters.find(currentToken->type) !=
                                                        statementStarters.end())
        {
            syntaxError("Missing ;");
        }
    }
}

Node *Parser::parseRepeatStatement()
{
    // The current token should now be REPEAT.

    // Create a LOOP node.
    Node *loopNode = new Node(LOOP);
//    cout << "start of repeat" << endl;
    currentToken = scanner->nextToken();  // consume REPEAT

    parseStatementList(loopNode, UNTIL);

    if (currentToken->type == UNTIL)
    {
//    	cout << "test node" << endl;
        // Create a TEST node. It adopts the test expression node.
        Node *testNode = new Node(TEST);
        lineNumber = currentToken->lineNumber;
        testNode->lineNumber = lineNumber;
        currentToken = scanner->nextToken();  // consume UNTIL
        testNode->adopt(parseExpression());

        // The LOOP node adopts the TEST node as its final child.
        loopNode->adopt(testNode);
    }
    else syntaxError("Expecting UNTIL");

    return loopNode;
}

Node *Parser::parseWhileStatement()
{
//	cout << "parse while statement" << endl;
	// Current token should be WHILE

	// Create LOOP node
	Node *loopNode = new Node(LOOP);
	currentToken = scanner->nextToken();  // consume WHILE

	// Create TEST node
//	cout << "creating test : " << currentToken->text << endl;
	Node *testNode = new Node(TEST);
	Node *notNode = new Node(NodeType::NOT);

	notNode->adopt(parseExpression());
	// no need to consume since WHILE is already consumed
	testNode->adopt(notNode);

//	testNode->adopt(parseExpression());

//	lineNumber = currentToken->lineNumber;
//	testNode->lineNumber = lineNumber;

	// LOOP node adopts TEST node as first child
	loopNode->adopt(testNode);

	if (currentToken->type == DO)
	{
//		cout << "current token type is do" << endl;
		currentToken = scanner->nextToken(); // consume DO
//		cout << "token after DO : " << currentToken->text << endl;
		loopNode->adopt(parseStatement());
	}
	else
	{
//		cout << "not do" << endl;
//		cout << currentToken->text << endl;
		syntaxError("Expecting DO");
	}

	return loopNode;

}

Node *Parser::parseWriteStatement()
{
    // The current token should now be WRITE.

    // Create a WRITE node-> It adopts the variable or string node.
    Node *writeNode = new Node(NodeType::WRITE);
    currentToken = scanner->nextToken();  // consume WRITE

    parseWriteArguments(writeNode);
    if (writeNode->children.size() == 0)
    {
        syntaxError("Invalid WRITE statement");
    }

    return writeNode;
}

Node *Parser::parseWritelnStatement()
{
    // The current token should now be WRITELN.

    // Create a WRITELN node. It adopts the variable or string node.
    Node *writelnNode = new Node(NodeType::WRITELN);
    currentToken = scanner->nextToken();  // consume WRITELN

    if (currentToken->type == LPAREN) parseWriteArguments(writelnNode);
    return writelnNode;
}

void Parser::parseWriteArguments(Node *node)
{
    // The current token should now be (

//	cout << "parse write arguments : " << currentToken->text << " : " << currentToken->lineNumber << endl;

    bool hasArgument = false;

    if (currentToken->type == LPAREN)
    {
        currentToken = scanner->nextToken();  // consume after (
//        cout << "consume after ( : " << currentToken->text << endl;
    }
    else syntaxError("Missing left parenthesis");

    if (currentToken->type == IDENTIFIER)
    {
//    	cout << "current token identifier : " << currentToken->text << endl;
        node->adopt(parseVariable());
//        cout << "current token after parseVariable : " << currentToken->text << endl;
        hasArgument = true;
    }
    else if (   (currentToken->type == CHARACTER)
             || (currentToken->type == STRING))
    {
        node->adopt(parseStringConstant());
        hasArgument = true;
    }
    else syntaxError("Invalid WRITE or WRITELN statement");

    // Look for a field width and a count of decimal places.
    if (hasArgument)
    {
        if (currentToken->type == COLON)
        {
            currentToken = scanner->nextToken();  // consume ,

            if (currentToken->type == INTEGER)
            {
                // Field width
                node->adopt(parseIntegerConstant());

                if (currentToken->type == COLON)
                {
                    currentToken = scanner->nextToken();  // consume ,

                    if (currentToken->type == INTEGER)
                    {
                        // Count of decimal places
                        node->adopt(parseIntegerConstant());
                    }
                    else syntaxError("Invalid count of decimal places");
                }
            }
            else syntaxError("Invalid field width");
        }
    }

    if (currentToken->type == RPAREN)
    {
        currentToken = scanner->nextToken();  // consume )
    }
    else syntaxError("Missing right parenthesis");
}

Node *Parser::parseExpression()
{
    // The current token should now be an identifier or a number.

    // The expression's root node->
    Node *exprNode = parseSimpleExpression();

    // The current token might now be a relational operator.
    if (relationalOperators.find(currentToken->type) != relationalOperators.end())
    {
        TokenType tokenType = currentToken->type;
//        cout << "parse expression : " << currentToken->text << " : " << currentToken->lineNumber << endl;
        Node *opNode = tokenType == EQUALS    ? new Node(EQ)
                    : tokenType == LESS_THAN ? new Node(LT)
        			: tokenType == LESS_EQUALS ? new Node(LE)
        			: tokenType == GREATER_THAN ? new Node(GT)
        			: tokenType == GREATER_EQUALS ? new Node(GE)
        			: tokenType == NOT_EQUALS ? new Node(NE)
                    :                          nullptr;

        currentToken = scanner->nextToken();  // consume relational operator

        // The relational operator node adopts the first simple expression
        // node as its first child and the second simple expression node
        // as its second child. Then it becomes the expression's root node.
        if (opNode != nullptr)
        {
            opNode->adopt(exprNode);
            opNode->adopt(parseSimpleExpression());
            exprNode = opNode;
        }
    }

    return exprNode;
}

Node *Parser::parseSimpleExpression()
{
    // The current token should now be an identifier or a number.

    // The simple expression's root node->
    Node *simpExprNode = parseTerm();

    // Keep parsing more terms as long as the current token
    // is a + or - operator.
    while (simpleExpressionOperators.find(currentToken->type) !=
                                                simpleExpressionOperators.end())
    {
        Node *opNode = currentToken->type == PLUS ? new Node(ADD)
                                                : new Node(SUBTRACT);

        currentToken = scanner->nextToken();  // consume the operator

        // The add or subtract node adopts the first term node as its
        // first child and the next term node as its second child.
        // Then it becomes the simple expression's root node.
        opNode->adopt(simpExprNode);
        opNode->adopt(parseTerm());
        simpExprNode = opNode;
    }

    return simpExprNode;
}

Node *Parser::parseTerm()
{
    // The current token should now be an identifier or a number.

    // The term's root node->
    Node *termNode = parseFactor();

    // Keep parsing more factors as long as the current token
    // is a * or / operator.
    while (termOperators.find(currentToken->type) != termOperators.end())
    {
        Node *opNode = currentToken->type == STAR ? new Node(MULTIPLY)
                                                : new Node(DIVIDE);

        currentToken = scanner->nextToken();  // consume the operator

        // The multiply or divide node adopts the first factor node as its
        // as its first child and the next factor node as its second child.
        // Then it becomes the term's root node.
        opNode->adopt(termNode);
        opNode->adopt(parseFactor());
        termNode = opNode;
    }

    return termNode;
}

Node *Parser::parseFactor()
{
    // The current token should now be an identifier or a number or (

    if      (currentToken->type == IDENTIFIER) return parseVariable();
    else if (currentToken->type == INTEGER)    return parseIntegerConstant();
    else if (currentToken->type == REAL)       return parseRealConstant();

    else if (currentToken->type == LPAREN)
    {
        currentToken = scanner->nextToken();  // consume (
        Node *exprNode = parseExpression();

        if (currentToken->type == RPAREN)
        {
            currentToken = scanner->nextToken();  // consume )
        }
        else syntaxError("Expecting )");

        return exprNode;
    }

    else syntaxError("Unexpected token");
    return nullptr;
}

Node *Parser::parseVariable()
{
    // The current token should now be an identifier.

    // Has the variable been "declared"?
    string variableName = currentToken->text;
    SymtabEntry *variableId = symtab->lookup(toLowerCase(variableName));
    if (variableId == nullptr) semanticError("Undeclared identifier");

    Node *node  = new Node(VARIABLE);
    node->text  = variableName;
    node->entry = variableId;

    currentToken = scanner->nextToken();  // consume the identifier
    return node;
}

Node *Parser::parseIntegerConstant()
{
    // The current token should now be a number.

    Node *integerNode = new Node(INTEGER_CONSTANT);
    integerNode->value = currentToken->value;

    currentToken = scanner->nextToken();  // consume the number
    return integerNode;
}

Node *Parser::parseRealConstant()
{
    // The current token should now be a number.

    Node *realNode = new Node(REAL_CONSTANT);
    realNode->value = currentToken->value;

    currentToken = scanner->nextToken();  // consume the number
    return realNode;
}

Node *Parser::parseStringConstant()
{
    // The current token should now be string.

    Node *stringNode = new Node(STRING_CONSTANT);
    stringNode->value = currentToken->value;

    currentToken = scanner->nextToken();  // consume the string
    return stringNode;
}

void Parser::syntaxError(string message)
{
    printf("SYNTAX ERROR at line %d: %s at '%s'\n",
           lineNumber, message.c_str(), currentToken->text.c_str());
    errorCount++;

    // Recover by skipping the rest of the statement.
    // Skip to a statement follower token.
    while (statementFollowers.find(currentToken->type) ==
                                                    statementFollowers.end())
    {
        currentToken = scanner->nextToken();
    }
//    exit(-1);
}

void Parser::semanticError(string message)
{
    printf("SEMANTIC ERROR at line %d: %s at '%s'\n",
           lineNumber, message.c_str(), currentToken->text.c_str());
    errorCount++;
}

}  // namespace frontend
