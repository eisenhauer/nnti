
#include "Teuchos_RefCountPtr.hpp"
#include<iostream>

using namespace Teuchos;

// Dummy class to contain a double 
// ------------------------------------------------------------
class Bar 
{
  public:
    // Constructor
    Bar();
    Bar(double x);
    // Destructor
    ~Bar();
    void setx(double x);
    double getx() const; 
  protected:
    double x_;
};
Bar::Bar()
{ 
  x_ = 5.0; 
};
Bar::Bar(double x)
{ 
  x_ = x; 
};
Bar::~Bar() 
{ 
  x_ = 0.0; 
};
void Bar::setx(double x)
{
  x_ = x;
};
double Bar::getx() const
{ 
  return(x_); 
};
// ------------------------------------------------------------

// Class to test out const
class Foo
{
  public:
    Foo();
    ~Foo();
    void setBar(const RefCountPtr<Bar> &Bptr);
    void setConstBar(const RefCountPtr<const Bar> &Bptr);
    void test1(const RefCountPtr<Bar> &Bptr);
    void test2();
    void test3(RefCountPtr<Bar> &Bptr);
    void test4(const RefCountPtr<Bar> &Bptr);
    RefCountPtr<Bar> &test5();
    RefCountPtr<const Bar> &test6();
    const RefCountPtr<Bar> &test7();
    const RefCountPtr<Bar> test8();
    RefCountPtr<Bar> test9();
    RefCountPtr<Bar> &test10();

  protected:
    double t_;
    RefCountPtr<Bar> Bptr_;
    RefCountPtr<const Bar> BptrConst_;

};
Foo::Foo() { };
Foo::~Foo() { };
void Foo::setBar(const RefCountPtr<Bar> &Bptr) 
{ 
  Bptr_ = Bptr;  
};
void Foo::setConstBar(const RefCountPtr<const Bar> &Bptr) 
{ 
  BptrConst_ = Bptr; 
  //Bptr_ = Bptr;  // not allowed because Bptr_ is nonconst
  //Bptr->setx(12.0); // not allowed because Bptr is const Bar RefCountPtr
};
void Foo::test1(const RefCountPtr<Bar> &Bptr)
{
  Bptr->setx(15.0);
};
void Foo::test2()
{
//  BptrConst_->setx(20.0); // not allowed because BptrConst_ is const
};
void Foo::test3(RefCountPtr<Bar> &Bptr)
{
  RefCountPtr<Bar> Bptr_temp = rcp(new Bar);
  Bptr_temp->setx(25.0);
  Bptr = Bptr_temp;
};
void Foo::test4(const RefCountPtr<Bar> &Bptr)
{
  RefCountPtr<Bar> Bptr_temp = rcp(new Bar);
  Bptr_temp->setx(30.0);
//  Bptr = Bptr_temp; // not allowed because input is const
};
RefCountPtr<Bar> &Foo::test5()
{
  return(Bptr_);
};
RefCountPtr<const Bar> &Foo::test6()
{
  //return(Bptr_); // not allowed because Bptr is nonconst
  return(BptrConst_);
};
const RefCountPtr<Bar> &Foo::test7()
{
  return(Bptr_);
};
const RefCountPtr<Bar> Foo::test8()
{
  return(rcp(new Bar(45.0)));
};
RefCountPtr<Bar> Foo::test9()
{
  return(rcp(new Bar(55.0)));
};
RefCountPtr<Bar> &Foo::test10()
{
  return(Bptr_);
};
// ------------------------------------------------------------


int main(int argc, char *argv[])
{
  cout << "main:  This routine tests const conditions." << endl;

  Foo F;
  RefCountPtr<Bar> Bptr = rcp(new Bar);
  Bptr->setx(10.0);
  F.setBar(Bptr);
  cout << "Correct output is x = 10." << endl;
  cout << "x = " << Bptr->getx() << endl;

  F.test1(Bptr);
  cout << "Correct output is x = 15." << endl;
  cout << "x = " << Bptr->getx() << endl;

  F.test3(Bptr);
  cout << "Correct output is x = 25." << endl;
  cout << "x = " << Bptr->getx() << endl;

  Bptr = F.test5();
  cout << "Correct output is x = 15." << endl;
  cout << "x = " << Bptr->getx() << endl;

  Bptr->setx(35.0);
//  Bptr = F.test6(); // not allowed because Bptr is nonconst and output of test6 is const
  RefCountPtr<const Bar> BptrConst = rcp(new Bar(40.0));
  F.setConstBar(BptrConst);
  cout << "Correct output is x = 40." << endl;
  cout << "x = " << F.test6()->getx() << endl;  // valid because we put const after Bar::getx

  Bptr = F.test7();
  cout << "Correct output is x = 35." << endl;
  cout << "x = " << Bptr->getx() << endl;  

  Bptr = F.test8();
  cout << "Correct output is x = 45." << endl;
  cout << "x = " << Bptr->getx() << endl;  
  //F.test8() = rcp(new Bar(50.0)); // not allowed because output is const.

  F.test9() = rcp(new Bar(60.0)); // allowed because output is nonconst, but this is crazy.

  F.test10() = rcp(new Bar(65.0));  // allowed because output is nonconst, and
  //this does something strange, as it modifies the internal data to the class
  //  through a nonconst reference passed out.
  Bptr = F.test7(); // grab it back out with const output
  cout << "Correct output is x = 65." << endl;
  cout << "x = " << Bptr->getx() << endl;  

//  F.test7() = rcp(new Bar(70.0));  // not allowed because output is const, this
  // is the expected behavior.  You shouldn't be able to do assignments like this.

  return(0);
};
