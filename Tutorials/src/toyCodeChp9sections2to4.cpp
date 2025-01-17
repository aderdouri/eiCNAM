/*
	Toy code contained in chapter 9, sections 2, 3 and 4 (pages 328 to 349).

	For VS15, the code in the book compiles and reproduces the results described.
	VS17 has issues with not accepting directly created lambdas into non const refs, so the code below is modified slightly
	to work in both VS15 and VS17.
*/
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <queue>
using namespace std;

/******************************************************************************************************************/
class Node
{
protected:
	vector<shared_ptr<Node>> myArguments;

	bool myProcessed = false;
	unsigned myOrder = 0;
	double myResult;
	double myAdjoint = 0.0;

public:
	virtual ~Node() {};

	// visitFunc:
	// a function of Node& that conducts a particular form of visit
	// templated so we can pass a lambda
	template <class V>
	void postOrder(V &visitFunc)
	{
		// Already processed -> do nothing
		if (myProcessed == false)
		{
			// Process children first
			for (auto argument : myArguments)
				argument->postOrder(visitFunc);

			// Visit the node
			visitFunc(*this);

			// Mark as processed
			myProcessed = true;
		}
	}

	template <class V>
	void preOrder(V &visitFunc)
	{
		// Visit the node first
		visitFunc(*this);

		// Process children
		for (auto argument : myArguments)
			argument->preOrder(visitFunc);
	}

	// Additional function required to handle VS17 compatibility
	template <class V>
	void breadthFirst(V &visitFunc)
	{
		queue<shared_ptr<Node>> q;
		breadthFirst(visitFunc, q);
	}

	// The signature of this function has changed to work in both VS15 and VS17.
	template <class V>
	void breadthFirst(
		V &visitFunc,
		queue<shared_ptr<Node>> &q)
	{
		// Send kids to queue
		for (auto argument : myArguments)
			q.push(argument);

		// Visit the node first
		visitFunc(*this);

		// Process nodes in the queue until empty
		while (!q.empty())
		{
			// Access the front node
			auto n = q.front();

			// Remove from queue
			q.pop();

			// Process it
			n->breadthFirst(visitFunc, q);
		} // Finished processing the queue: exit
	}

	// Access result
	double result()
	{
		return myResult;
	}

	// Reset processsed flags
	void resetProcessed()
	{
		for (auto argument : myArguments)
			argument->resetProcessed();
		myProcessed = false;
	}

	virtual void evaluate() = 0;

	void setOrder(unsigned order)
	{
		myOrder = order;
	}

	unsigned order()
	{
		return myOrder;
	}

	virtual void logInstruction() = 0;

	// Note: return by reference
	// used to get and set
	double &adjoint()
	{
		return myAdjoint;
	}

	void resetAdjoints()
	{
		for (auto argument : myArguments)
			argument->resetAdjoints();
		myAdjoint = 0.0;
	}

	virtual void propagateAdjoint() = 0;
};

/******************************************************************************************************************/
class PlusNode : public Node
{
public:
	PlusNode(shared_ptr<Node> lhs, shared_ptr<Node> rhs)
	{
		myArguments.resize(2);
		myArguments[0] = lhs;
		myArguments[1] = rhs;
	}

	void evaluate() override
	{
		myResult = myArguments[0]->result() + myArguments[1]->result();
	}

	void logInstruction() override
	{
		cout << "y" << order() << " = y" << myArguments[0]->order() << " + y" << myArguments[1]->order() << endl;
	}

	void propagateAdjoint() override
	{
		cout << "Propogating node " << myOrder
			 << " adjoint = " << myAdjoint << endl;

		myArguments[0]->adjoint() += myAdjoint;
		myArguments[1]->adjoint() += myAdjoint;

		// Set the adjoint to 0 after its propagation to child nodes (see section 9.4, page 347).
		myAdjoint = 0.0;
	}
};

/******************************************************************************************************************/
class TimesNode : public Node
{
public:
	TimesNode(shared_ptr<Node> lhs, shared_ptr<Node> rhs)
	{
		myArguments.resize(2);
		myArguments[0] = lhs;
		myArguments[1] = rhs;
	}

	void evaluate() override
	{
		myResult = myArguments[0]->result() * myArguments[1]->result();
	}

	void logInstruction() override
	{
		cout << "y" << order() << " = y" << myArguments[0]->order() << " * y" << myArguments[1]->order() << endl;
	}

	void propagateAdjoint() override
	{
		cout << "Propogating node " << myOrder
			 << " adjoint = " << myAdjoint << endl;

		myArguments[0]->adjoint() += myAdjoint * myArguments[1]->result();
		myArguments[1]->adjoint() += myAdjoint * myArguments[0]->result();

		// Set the adjoint to 0 after its propagation to child nodes (see section 9.4, page 347).
		myAdjoint = 0.0;
	}
};

/******************************************************************************************************************/
class LogNode : public Node
{
public:
	LogNode(shared_ptr<Node> arg)
	{
		myArguments.resize(1);
		myArguments[0] = arg;
	}

	void evaluate() override
	{
		myResult = log(myArguments[0]->result());
	}

	void logInstruction() override
	{
		cout << "y" << order() << " = log(y" << myArguments[0]->order() << ")" << endl;
	}

	void propagateAdjoint() override
	{
		cout << "Propogating node " << myOrder
			 << " adjoint = " << myAdjoint << endl;

		myArguments[0]->adjoint() += myAdjoint / myArguments[0]->result();

		// Set the adjoint to 0 after its propagation to child nodes (see section 9.4, page 347).
		myAdjoint = 0.0;
	}
};

/******************************************************************************************************************/
class Leaf : public Node
{
	double myValue;

public:
	Leaf(double val) : myValue(val) {}

	double getVal()
	{
		return myValue;
	}

	void setVal(double val)
	{
		myValue = val;
	}

	void evaluate() override
	{
		myResult = myValue;
	}

	void logInstruction() override
	{
		cout << "y" << order() << " = " << myValue << endl;
	}

	void propagateAdjoint() override
	{
		cout << "Accumulating leaf " << myOrder
			 << " adjoint = " << myAdjoint << endl;
	}
};

/******************************************************************************************************************/
class Number
{
	shared_ptr<Node> myNode;

public:
	Number(double val) : myNode(new Leaf(val)) {}

	Number(shared_ptr<Node> node) : myNode(node) {}

	shared_ptr<Node> node() { return myNode; }

	void setVal(double val)
	{
		// Cast to Leaf, only leaves can be changed
		dynamic_pointer_cast<Leaf>(myNode)->setVal(val);
	}

	double getVal()
	{
		// Only leaves can be read
		return dynamic_pointer_cast<Leaf>(myNode)->getVal();
	}

	double evaluate()
	{
		myNode->resetProcessed();
		// The next line is the original code that works in VS15
		// myNode->postOrder([](Node& n) {n.evaluate(); });
		// The next two lines work in both VS15 and VS17
		auto lam = [](Node &n) mutable
		{ n.evaluate(); };
		myNode->postOrder(lam);
		return myNode->result();
	}

	void setOrder()
	{
		myNode->resetProcessed();
		// The next line is the original code that works in VS15
		// myNode->postOrder([order = 0](Node& n) mutable {n.setOrder(++order); });
		// The next two lines work in both VS15 and VS17
		auto lam = [order = 0](Node &n) mutable
		{ n.setOrder(++order); };
		myNode->postOrder(lam);
	}

	void logResults()
	{
		myNode->resetProcessed();
		// The next line is the original code that works in VS15
		// myNode->postOrder([](Node& n) {cout << "Processed node " << n.order() << " result " << n.result() << endl; });
		// The next two lines work in both VS15 and VS17
		auto lam = [order = 0](Node &n) mutable
		{ cout << "Processed node " << n.order() << " result " << n.result() << endl; };
		myNode->postOrder(lam);
	}

	void logProgram()
	{
		myNode->resetProcessed();
		// The next line is the original code that works in VS15
		// myNode->postOrder([](Node& n) {n.logInstruction(); });
		// The next two lines work in both VS15 and VS17
		auto lam = [order = 0](Node &n) mutable
		{ n.logInstruction(); };
		myNode->postOrder(lam);
	}

	// Accessor/setter, from the inputs
	double &adjoint()
	{
		return myNode->adjoint();
	}

	// Propagator, from the result
	void propagateAdjoints()
	{
		myNode->resetAdjoints();
		myNode->adjoint() = 1.0;
		// Preorder traversal code
		// The next line is the original code that works in VS15
		// myNode->preOrder([](Node& n) {n.propagateAdjoint(); });
		// The next two lines work in both VS15 and VS17
		auto lam = [](Node &n)
		{ n.propagateAdjoint(); };
		myNode->preOrder(lam);

		// Breadth-first traversal code
		// The next line is the original code that works in VS15
		// myNode->breadthFirst([](Node& n) {n.propagateAdjoint(); });
		// The next two lines work in both VS15 and VS17
		// auto lam = [](Node& n) {n.propagateAdjoint(); };
		// myNode->breadthFirst(lam);
	}
};

shared_ptr<Node> operator+(Number lhs, Number rhs)
{
	return shared_ptr<Node>(new PlusNode(lhs.node(), rhs.node()));
}

shared_ptr<Node> operator*(Number lhs, Number rhs)
{
	return shared_ptr<Node>(new TimesNode(lhs.node(), rhs.node()));
}

shared_ptr<Node> log(Number arg)
{
	return shared_ptr<Node>(new LogNode(arg.node()));
}

/******************************************************************************************************************/
/*
	Templated calculation code (see section 9.2, page 331)
*/
template <class T>
T f(T x[5])
{
	T y1 = x[2] * (5.0 * x[0] + x[1]);
	T y2 = log(y1);
	T y = (y1 + x[3] * y2) * (y1 + y2);
	return y;
}

/******************************************************************************************************************/
int main()
{
	Number x[5] = {1.0, 2.0, 3.0, 4.0, 5.0};

	// Build the DAG
	Number y = f(x);

	// Set the order on the DAG
	y.setOrder();

	// Log program
	y.logProgram();

	// Evaluate on the DAG
	cout << y.evaluate() << endl; // 797.751

	// Log all results
	y.logResults();

	// Uncomment the following code to evaluate the DAG with a different input (see page 336)
	/*
	// Change x0 on the DAG
	x[0].setVal(2.5);

	// Evaluate on the DAG again
	cout << y.evaluate() << endl; // 2769.76

	// Log results again
	y.logResults();*/

	y.propagateAdjoints();

	// Get derivatives
	for (size_t i = 0; i < 5; ++i)
	{
		cout << "a" << i << " = " << x[i].adjoint() << endl;
	}

	return 0;
}
