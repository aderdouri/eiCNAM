/*
	Toy code contained in chapter 15, section 2 (pages 512 to 520). 

	The code is modified slightly to hold the adjoint in the class Number within a shared pointer to ensure that 
	all copies of Number hold the same adjoint (thereby allowing the adjoints to be updated within x1 and x2).
*/
#include <iostream>
#include <string>
using namespace std;

/************************************************************************************************************************************************************/
template <class E>
class Expression
{};

/************************************************************************************************************************************************************/
// CRTP (Curiously Recurring Template Pattern)
template <class LHS, class RHS>
class ExprTimes : public Expression<ExprTimes<LHS, RHS>>
{
	LHS lhs;
	RHS rhs;

public:

	// Constructor
	explicit ExprTimes
		(const Expression<LHS>& l, const Expression<RHS>& r)
		: lhs(static_cast<const LHS&>(l)), 
		rhs(static_cast<const RHS&>(r)) {}

	double value() const
	{
		return lhs.value() * rhs.value();
	}

	enum { numNumbers = LHS::numNumbers + RHS::numNumbers };

	string writeProgram(
		// On input, the number of nodes processed so far
		// On return the total number of nodes processed on exit
		size_t& processed)
	{
		// Process the left sub-DAG
		const string ls = lhs.writeProgram(processed);
		const size_t ln = processed - 1;

		// Process the right sub-DAG
		const string rs = rhs.writeProgram(processed);
		const size_t rn = processed - 1;

		// Process this node
		const string thisString = ls + rs + 
			"y" + to_string(processed) 
			+ " = y" + to_string(ln) + " * y" + to_string(rn) + "\n";

		++processed;

		return thisString;
	}

	// Input: accumulated adjoint for this node or 1 if top node
	void pushAdjoint(const double adjoint)
	{
		lhs.pushAdjoint(adjoint * rhs.value());
		rhs.pushAdjoint(adjoint * lhs.value());
	}
};

// Operator overload for expressions
template <class LHS, class RHS>
inline ExprTimes<LHS, RHS> operator*(
	const Expression<LHS>& lhs, const Expression<RHS>& rhs)
{
	return ExprTimes<LHS, RHS>(lhs, rhs);
}

/************************************************************************************************************************************************************/
template <class ARG>
class ExprLog : public Expression<ExprLog<ARG>>
{
	ARG arg;

public:

	// Constructor
	explicit ExprLog(const Expression<ARG>& a)
		: arg(static_cast<const ARG&>(a)) {}

	double value() const
	{
		return log(arg.value());
	}

	enum { numNumbers = ARG::numNumbers };

	string writeProgram(
		// On input, the number of nodes processed so far
		// On return the total number of nodes processed on exit
		size_t& processed)
	{
		// Process the arg sub-DAG
		const string s = arg.writeProgram(processed);
		const size_t n = processed - 1;

		// Process this node
		const string thisString = s + 
			"y" + to_string(processed) 
			+ " = log(y" + to_string(n) + ")\n";

		++processed;

		return thisString;
	}

	// Input: accumulated adjoint for this node or 1 if top node
	void pushAdjoint(const double adjoint)
	{
		arg.pushAdjoint(adjoint / arg.value());
	}
};

// Operator overload for expressions
template <class ARG>
inline ExprLog<ARG> log(const Expression<ARG>& arg)
{
	return ExprLog<ARG>(arg);
}

/************************************************************************************************************************************************************/
// Number type, also an expression
class Number : public Expression<Number>
{
	double val;
	shared_ptr<double> adj; // Use a shared pointer so that copies of Number hold the same adjoint value (required to make lines 199 and 200 work)

public:

	// Constructor
	explicit Number(const double v) : val(v), adj(make_shared<double>(0.0)) {}	// code modified from text to work with shared pointer

	double value() const
	{
		return val;
	}

	double adjoint() const
	{
		return *adj;	// code modified from text to work with shared pointer
	}

	enum { numNumbers = 1 };

	string writeProgram(		
		// On input, the number of nodes processed so far
		// On return the total number of nodes processed on exit
		size_t& processed)
	{
		const string thisString = "y" + to_string(processed) + " = " + to_string(val) + "\n";

		++processed;

		return thisString;
	}

	void pushAdjoint(const double adjoint)
	{
		*adj = adjoint;		// code modified from text to work with shared pointer
	}
};

/************************************************************************************************************************************************************/
auto calculate(Number t1, Number t2)
{
	return t1 * log(t2);
}

/************************************************************************************************************************************************************/
template <class E>
constexpr auto countNumbersIn(const Expression<E>&)
{
	return E::numNumbers;
}

int main()
{
	Number x1(2.0), x2(3.0);

	auto e = calculate(x1, x2);

    std::cout << e.value() << std::endl;	// 2.19722 = 2 * log(3)

	std::cout << countNumbersIn(e) << std::endl;	// 2 - note this fixes a typo in the text that used numNumbersIn

	size_t processed = 0;
	cout << e.writeProgram(processed);

	e.pushAdjoint(1.0);
	cout << "x1 adjoint = " << x1.adjoint() << endl;	// 1.09861 = log(3)
	cout << "x2 adjoint = " << x2.adjoint() << endl;	// 0.666667 = 2/3

	return 0;
}
