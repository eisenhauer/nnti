#ifndef _tinyfadsixteen_h_
#define _tinyfadsixteen_h_

//*********************************************************
// This file is generated by generate.cc.
// Use this file for any modification
//*********************************************************

template <class T> class TinyFad<16,T> {
public:
  typedef T value_type;
  typedef T grad_type;
protected:

  int n;
  T val_;

  T dx0_;
  T dx1_;
  T dx2_;
  T dx3_;
  T dx4_;
  T dx5_;
  T dx6_;
  T dx7_;
  T dx8_;
  T dx9_;
  T dx10_;
  T dx11_;
  T dx12_;
  T dx13_;
  T dx14_;
  T dx15_;


public:
  void diff(const size_t ith, const size_t sz){
     n = ith+1;
     dx0_ = T(0.);
     dx1_ = T(0.);
     dx2_ = T(0.);
     dx3_ = T(0.);
     dx4_ = T(0.);
     dx5_ = T(0.);
     dx6_ = T(0.);
     dx7_ = T(0.);
     dx8_ = T(0.);
     dx9_ = T(0.);
     dx10_ = T(0.);
     dx11_ = T(0.);
     dx12_ = T(0.);
     dx13_ = T(0.);
     dx14_ = T(0.);
     dx15_ = T(0.);

     switch(ith){
     case 0 : dx0_ = T(1.);break;
     case 1 : dx1_ = T(1.);break;
     case 2 : dx2_ = T(1.);break;
     case 3 : dx3_ = T(1.);break;
     case 4 : dx4_ = T(1.);break;
     case 5 : dx5_ = T(1.);break;
     case 6 : dx6_ = T(1.);break;
     case 7 : dx7_ = T(1.);break;
     case 8 : dx8_ = T(1.);break;
     case 9 : dx9_ = T(1.);break;
     case 10 : dx10_ = T(1.);break;
     case 11 : dx11_ = T(1.);break;
     case 12 : dx12_ = T(1.);break;
     case 13 : dx13_ = T(1.);break;
     case 14 : dx14_ = T(1.);break;
     case 15 : dx15_ = T(1.);break;
     default : cout << "ith = " << ith << "  out of definition set" << endl;exit(1);
     }
  }

  TinyFad(const T& ind, const int ini) : n(ini+1), val_(ind) {
     dx0_ = T(0.);
     dx1_ = T(0.);
     dx2_ = T(0.);
     dx3_ = T(0.);
     dx4_ = T(0.);
     dx5_ = T(0.);
     dx6_ = T(0.);
     dx7_ = T(0.);
     dx8_ = T(0.);
     dx9_ = T(0.);
     dx10_ = T(0.);
     dx11_ = T(0.);
     dx12_ = T(0.);
     dx13_ = T(0.);
     dx14_ = T(0.);
     dx15_ = T(0.);

     switch(ini){
     case 0 : dx0_ = T(1.);break;
     case 1 : dx1_ = T(1.);break;
     case 2 : dx2_ = T(1.);break;
     case 3 : dx3_ = T(1.);break;
     case 4 : dx4_ = T(1.);break;
     case 5 : dx5_ = T(1.);break;
     case 6 : dx6_ = T(1.);break;
     case 7 : dx7_ = T(1.);break;
     case 8 : dx8_ = T(1.);break;
     case 9 : dx9_ = T(1.);break;
     case 10 : dx10_ = T(1.);break;
     case 11 : dx11_ = T(1.);break;
     case 12 : dx12_ = T(1.);break;
     case 13 : dx13_ = T(1.);break;
     case 14 : dx14_ = T(1.);break;
     case 15 : dx15_ = T(1.);break;
     default : cout << "ini = " << ini << "  out of definition set" << endl;exit(1);
     }
  }
  TinyFad() : n(0), val_(0.) {
     dx0_ = T(0.);
     dx1_ = T(0.);
     dx2_ = T(0.);
     dx3_ = T(0.);
     dx4_ = T(0.);
     dx5_ = T(0.);
     dx6_ = T(0.);
     dx7_ = T(0.);
     dx8_ = T(0.);
     dx9_ = T(0.);
     dx10_ = T(0.);
     dx11_ = T(0.);
     dx12_ = T(0.);
     dx13_ = T(0.);
     dx14_ = T(0.);
     dx15_ = T(0.);
  }
  TinyFad(const No_Initialization &): n(0) {}
  TinyFad(const T& in) : n(0), val_(in) {
     dx0_ = T(0.);
     dx1_ = T(0.);
     dx2_ = T(0.);
     dx3_ = T(0.);
     dx4_ = T(0.);
     dx5_ = T(0.);
     dx6_ = T(0.);
     dx7_ = T(0.);
     dx8_ = T(0.);
     dx9_ = T(0.);
     dx10_ = T(0.);
     dx11_ = T(0.);
     dx12_ = T(0.);
     dx13_ = T(0.);
     dx14_ = T(0.);
     dx15_ = T(0.);
  }
  TinyFad(const TinyFad<16,T> & in) : n(0), val_(in.val_) {
     dx0_ = in.dx0_;
     dx1_ = in.dx1_;
     dx2_ = in.dx2_;
     dx3_ = in.dx3_;
     dx4_ = in.dx4_;
     dx5_ = in.dx5_;
     dx6_ = in.dx6_;
     dx7_ = in.dx7_;
     dx8_ = in.dx8_;
     dx9_ = in.dx9_;
     dx10_ = in.dx10_;
     dx11_ = in.dx11_;
     dx12_ = in.dx12_;
     dx13_ = in.dx13_;
     dx14_ = in.dx14_;
     dx15_ = in.dx15_;
  }

  ~TinyFad() {}

  int    N()       const {return n-1;}

  const T& val()     const { return val_;}
  T& val()                 { return val_;}

  const T& d0() const { return dx0_;}
  T& d0() { return dx0_;}

  const T& d1() const { return dx1_;}
  T& d1() { return dx1_;}

  const T& d2() const { return dx2_;}
  T& d2() { return dx2_;}

  const T& d3() const { return dx3_;}
  T& d3() { return dx3_;}

  const T& d4() const { return dx4_;}
  T& d4() { return dx4_;}

  const T& d5() const { return dx5_;}
  T& d5() { return dx5_;}

  const T& d6() const { return dx6_;}
  T& d6() { return dx6_;}

  const T& d7() const { return dx7_;}
  T& d7() { return dx7_;}

  const T& d8() const { return dx8_;}
  T& d8() { return dx8_;}

  const T& d9() const { return dx9_;}
  T& d9() { return dx9_;}

  const T& d10() const { return dx10_;}
  T& d10() { return dx10_;}

  const T& d11() const { return dx11_;}
  T& d11() { return dx11_;}

  const T& d12() const { return dx12_;}
  T& d12() { return dx12_;}

  const T& d13() const { return dx13_;}
  T& d13() { return dx13_;}

  const T& d14() const { return dx14_;}
  T& d14() { return dx14_;}

  const T& d15() const { return dx15_;}
  T& d15() { return dx15_;}

  T& dx(int i){
     switch(i){
     case 0 : return dx0_;
     case 1 : return dx1_;
     case 2 : return dx2_;
     case 3 : return dx3_;
     case 4 : return dx4_;
     case 5 : return dx5_;
     case 6 : return dx6_;
     case 7 : return dx7_;
     case 8 : return dx8_;
     case 9 : return dx9_;
     case 10 : return dx10_;
     case 11 : return dx11_;
     case 12 : return dx12_;
     case 13 : return dx13_;
     case 14 : return dx14_;
     case 15 : return dx15_;
     default : cout << "i out of bounds" << endl;exit(1);
     }
  }
  const T& dx(int i) const {
     switch(i){
     case 0 : return dx0_;
     case 1 : return dx1_;
     case 2 : return dx2_;
     case 3 : return dx3_;
     case 4 : return dx4_;
     case 5 : return dx5_;
     case 6 : return dx6_;
     case 7 : return dx7_;
     case 8 : return dx8_;
     case 9 : return dx9_;
     case 10 : return dx10_;
     case 11 : return dx11_;
     case 12 : return dx12_;
     case 13 : return dx13_;
     case 14 : return dx14_;
     case 15 : return dx15_;
     default : cout << "i out of bounds" << endl;exit(1);
     }
  }
  T& d(int i){
     switch(i){
     case 0 : return dx0_;
     case 1 : return dx1_;
     case 2 : return dx2_;
     case 3 : return dx3_;
     case 4 : return dx4_;
     case 5 : return dx5_;
     case 6 : return dx6_;
     case 7 : return dx7_;
     case 8 : return dx8_;
     case 9 : return dx9_;
     case 10 : return dx10_;
     case 11 : return dx11_;
     case 12 : return dx12_;
     case 13 : return dx13_;
     case 14 : return dx14_;
     case 15 : return dx15_;
     default : cout << "i out of bounds" << endl;exit(1);
     }
  }
  const T& d(int i) const {
     switch(i){
     case 0 : return dx0_;
     case 1 : return dx1_;
     case 2 : return dx2_;
     case 3 : return dx3_;
     case 4 : return dx4_;
     case 5 : return dx5_;
     case 6 : return dx6_;
     case 7 : return dx7_;
     case 8 : return dx8_;
     case 9 : return dx9_;
     case 10 : return dx10_;
     case 11 : return dx11_;
     case 12 : return dx12_;
     case 13 : return dx13_;
     case 14 : return dx14_;
     case 15 : return dx15_;
     default : cout << "i out of bounds" << endl;exit(1);
     }
  }

  TinyFad<16,T> & operator = (const TinyFad<16,T> & in){
     val_ = in.val_;

     dx0_ = in.dx0_;
     dx1_ = in.dx1_;
     dx2_ = in.dx2_;
     dx3_ = in.dx3_;
     dx4_ = in.dx4_;
     dx5_ = in.dx5_;
     dx6_ = in.dx6_;
     dx7_ = in.dx7_;
     dx8_ = in.dx8_;
     dx9_ = in.dx9_;
     dx10_ = in.dx10_;
     dx11_ = in.dx11_;
     dx12_ = in.dx12_;
     dx13_ = in.dx13_;
     dx14_ = in.dx14_;
     dx15_ = in.dx15_;

     return *this;
  }

  TinyFad<16,T> & operator = (const T & in){
     val_ = in;

     dx0_ = T(0.);
     dx1_ = T(0.);
     dx2_ = T(0.);
     dx3_ = T(0.);
     dx4_ = T(0.);
     dx5_ = T(0.);
     dx6_ = T(0.);
     dx7_ = T(0.);
     dx8_ = T(0.);
     dx9_ = T(0.);
     dx10_ = T(0.);
     dx11_ = T(0.);
     dx12_ = T(0.);
     dx13_ = T(0.);
     dx14_ = T(0.);
     dx15_ = T(0.);

     return *this;
  }

  TinyFad<16,T> & operator += (const TinyFad<16,T> & in){
     dx0_ += in.dx0_;
     dx1_ += in.dx1_;
     dx2_ += in.dx2_;
     dx3_ += in.dx3_;
     dx4_ += in.dx4_;
     dx5_ += in.dx5_;
     dx6_ += in.dx6_;
     dx7_ += in.dx7_;
     dx8_ += in.dx8_;
     dx9_ += in.dx9_;
     dx10_ += in.dx10_;
     dx11_ += in.dx11_;
     dx12_ += in.dx12_;
     dx13_ += in.dx13_;
     dx14_ += in.dx14_;
     dx15_ += in.dx15_;
     val_ += in.val_;


    return *this;
  }
  TinyFad<16,T> & operator -= (const TinyFad<16,T> & in){
     dx0_ -= in.dx0_;
     dx1_ -= in.dx1_;
     dx2_ -= in.dx2_;
     dx3_ -= in.dx3_;
     dx4_ -= in.dx4_;
     dx5_ -= in.dx5_;
     dx6_ -= in.dx6_;
     dx7_ -= in.dx7_;
     dx8_ -= in.dx8_;
     dx9_ -= in.dx9_;
     dx10_ -= in.dx10_;
     dx11_ -= in.dx11_;
     dx12_ -= in.dx12_;
     dx13_ -= in.dx13_;
     dx14_ -= in.dx14_;
     dx15_ -= in.dx15_;
     val_ -= in.val_;

     return *this;
  }
  TinyFad<16,T> & operator *= (const TinyFad<16,T> & in){
     dx0_ = dx0_ * in.val_ + val_ * in.dx0_;
     dx1_ = dx1_ * in.val_ + val_ * in.dx1_;
     dx2_ = dx2_ * in.val_ + val_ * in.dx2_;
     dx3_ = dx3_ * in.val_ + val_ * in.dx3_;
     dx4_ = dx4_ * in.val_ + val_ * in.dx4_;
     dx5_ = dx5_ * in.val_ + val_ * in.dx5_;
     dx6_ = dx6_ * in.val_ + val_ * in.dx6_;
     dx7_ = dx7_ * in.val_ + val_ * in.dx7_;
     dx8_ = dx8_ * in.val_ + val_ * in.dx8_;
     dx9_ = dx9_ * in.val_ + val_ * in.dx9_;
     dx10_ = dx10_ * in.val_ + val_ * in.dx10_;
     dx11_ = dx11_ * in.val_ + val_ * in.dx11_;
     dx12_ = dx12_ * in.val_ + val_ * in.dx12_;
     dx13_ = dx13_ * in.val_ + val_ * in.dx13_;
     dx14_ = dx14_ * in.val_ + val_ * in.dx14_;
     dx15_ = dx15_ * in.val_ + val_ * in.dx15_;
     val_ *= in.val_;

     return *this;
  }
  TinyFad<16,T> & operator /= (const TinyFad<16,T> & in){
     if (in.val_ == 0.) error("TinyFad & TinyFad::operator /= (const TinyFad & in), dividing by 0");
     dx0_ = ( dx0_ * in.val_ - val_ * in.dx0_ ) / in.val_ / in.val_ ;
     dx1_ = ( dx1_ * in.val_ - val_ * in.dx1_ ) / in.val_ / in.val_ ;
     dx2_ = ( dx2_ * in.val_ - val_ * in.dx2_ ) / in.val_ / in.val_ ;
     dx3_ = ( dx3_ * in.val_ - val_ * in.dx3_ ) / in.val_ / in.val_ ;
     dx4_ = ( dx4_ * in.val_ - val_ * in.dx4_ ) / in.val_ / in.val_ ;
     dx5_ = ( dx5_ * in.val_ - val_ * in.dx5_ ) / in.val_ / in.val_ ;
     dx6_ = ( dx6_ * in.val_ - val_ * in.dx6_ ) / in.val_ / in.val_ ;
     dx7_ = ( dx7_ * in.val_ - val_ * in.dx7_ ) / in.val_ / in.val_ ;
     dx8_ = ( dx8_ * in.val_ - val_ * in.dx8_ ) / in.val_ / in.val_ ;
     dx9_ = ( dx9_ * in.val_ - val_ * in.dx9_ ) / in.val_ / in.val_ ;
     dx10_ = ( dx10_ * in.val_ - val_ * in.dx10_ ) / in.val_ / in.val_ ;
     dx11_ = ( dx11_ * in.val_ - val_ * in.dx11_ ) / in.val_ / in.val_ ;
     dx12_ = ( dx12_ * in.val_ - val_ * in.dx12_ ) / in.val_ / in.val_ ;
     dx13_ = ( dx13_ * in.val_ - val_ * in.dx13_ ) / in.val_ / in.val_ ;
     dx14_ = ( dx14_ * in.val_ - val_ * in.dx14_ ) / in.val_ / in.val_ ;
     dx15_ = ( dx15_ * in.val_ - val_ * in.dx15_ ) / in.val_ / in.val_ ;
     val_ /= in.val_;

     return *this;
  }

  TinyFad<16,T> & operator += (const T & in){     val_ += in;

     return *this;
  }
  TinyFad<16,T> & operator -= (const T & in){     val_ -= in;

     return *this;
  }
  TinyFad<16,T> & operator *= (const T & in){
     val_ *= in;

     dx0_ *= in;
     dx1_ *= in;
     dx2_ *= in;
     dx3_ *= in;
     dx4_ *= in;
     dx5_ *= in;
     dx6_ *= in;
     dx7_ *= in;
     dx8_ *= in;
     dx9_ *= in;
     dx10_ *= in;
     dx11_ *= in;
     dx12_ *= in;
     dx13_ *= in;
     dx14_ *= in;
     dx15_ *= in;

     return *this;
  }
  TinyFad<16,T> & operator /= (const T & in){
     if ( in == T(0.) ) error("TinyFad & TinyFad::operator /= (const T & in), dividing by 0");
     val_ /= in;

     dx0_ /= in;
     dx1_ /= in;
     dx2_ /= in;
     dx3_ /= in;
     dx4_ /= in;
     dx5_ /= in;
     dx6_ /= in;
     dx7_ /= in;
     dx8_ /= in;
     dx9_ /= in;
     dx10_ /= in;
     dx11_ /= in;
     dx12_ /= in;
     dx13_ /= in;
     dx14_ /= in;
     dx15_ /= in;

     return *this;
  }

  TinyFad<16,T> operator++(int){
     TinyFad<16,T> tmp(*this);
     tmp.val_++;
     return tmp;
  };
  TinyFad<16,T> operator--(int){
     TinyFad<16,T> tmp(*this);
     tmp.val_--;
     return tmp;
  };
  TinyFad<16,T> & operator++(){     ++val_;
     return *this;
  }
  TinyFad<16,T> & operator--(){     --val_;
     return *this;
  }
};


template <class T> inline TinyFad<16,T> operator + (const TinyFad<16,T>& in)
{
  return TinyFad<16,T>(in);
}

template <class T> inline TinyFad<16,T> operator - (const TinyFad<16,T>& in)
{
  TinyFad<16,T> tmp;
  tmp -= in;
  return tmp;
}

template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote >
operator +(const TinyFad<16,L>& un, const TinyFad<16,R>& deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() = un.d0() + deux.d0();

  tmp.d1() = un.d1() + deux.d1();

  tmp.d2() = un.d2() + deux.d2();

  tmp.d3() = un.d3() + deux.d3();

  tmp.d4() = un.d4() + deux.d4();

  tmp.d5() = un.d5() + deux.d5();

  tmp.d6() = un.d6() + deux.d6();

  tmp.d7() = un.d7() + deux.d7();

  tmp.d8() = un.d8() + deux.d8();

  tmp.d9() = un.d9() + deux.d9();

  tmp.d10() = un.d10() + deux.d10();

  tmp.d11() = un.d11() + deux.d11();

  tmp.d12() = un.d12() + deux.d12();

  tmp.d13() = un.d13() + deux.d13();

  tmp.d14() = un.d14() + deux.d14();

  tmp.d15() = un.d15() + deux.d15();

  tmp.val() = un.val() + deux.val();

  return tmp;
}

template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote>
operator +(const TinyFad<16,L>& un, const R& deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() = un.d0();

  tmp.d1() = un.d1();

  tmp.d2() = un.d2();

  tmp.d3() = un.d3();

  tmp.d4() = un.d4();

  tmp.d5() = un.d5();

  tmp.d6() = un.d6();

  tmp.d7() = un.d7();

  tmp.d8() = un.d8();

  tmp.d9() = un.d9();

  tmp.d10() = un.d10();

  tmp.d11() = un.d11();

  tmp.d12() = un.d12();

  tmp.d13() = un.d13();

  tmp.d14() = un.d14();

  tmp.d15() = un.d15();

  tmp.val() = un.val() + deux;

  return tmp;
}

template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote >
operator +(const L& un, const TinyFad<16,R>& deux) {
  return operator +(deux,un);
}

template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote >
operator *(const TinyFad<16,L>& un, const TinyFad<16,R>& deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() = un.d0()*deux.val() + un.val() * deux.d0();

  tmp.d1() = un.d1()*deux.val() + un.val() * deux.d1();

  tmp.d2() = un.d2()*deux.val() + un.val() * deux.d2();

  tmp.d3() = un.d3()*deux.val() + un.val() * deux.d3();

  tmp.d4() = un.d4()*deux.val() + un.val() * deux.d4();

  tmp.d5() = un.d5()*deux.val() + un.val() * deux.d5();

  tmp.d6() = un.d6()*deux.val() + un.val() * deux.d6();

  tmp.d7() = un.d7()*deux.val() + un.val() * deux.d7();

  tmp.d8() = un.d8()*deux.val() + un.val() * deux.d8();

  tmp.d9() = un.d9()*deux.val() + un.val() * deux.d9();

  tmp.d10() = un.d10()*deux.val() + un.val() * deux.d10();

  tmp.d11() = un.d11()*deux.val() + un.val() * deux.d11();

  tmp.d12() = un.d12()*deux.val() + un.val() * deux.d12();

  tmp.d13() = un.d13()*deux.val() + un.val() * deux.d13();

  tmp.d14() = un.d14()*deux.val() + un.val() * deux.d14();

  tmp.d15() = un.d15()*deux.val() + un.val() * deux.d15();

  tmp.val() = un.val() * deux.val();

  return tmp;
}

template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote >
operator *(const TinyFad<16,L>& un, const R& deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() = un.d0()*deux;

  tmp.d1() = un.d1()*deux;

  tmp.d2() = un.d2()*deux;

  tmp.d3() = un.d3()*deux;

  tmp.d4() = un.d4()*deux;

  tmp.d5() = un.d5()*deux;

  tmp.d6() = un.d6()*deux;

  tmp.d7() = un.d7()*deux;

  tmp.d8() = un.d8()*deux;

  tmp.d9() = un.d9()*deux;

  tmp.d10() = un.d10()*deux;

  tmp.d11() = un.d11()*deux;

  tmp.d12() = un.d12()*deux;

  tmp.d13() = un.d13()*deux;

  tmp.d14() = un.d14()*deux;

  tmp.d15() = un.d15()*deux;

  tmp.val() = un.val() * deux;

  return tmp;
}

template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote >
operator *(const L& un, const TinyFad<16,R>& deux) {

  return operator *(deux,un);
}


template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote >
operator -(const TinyFad<16,L> & un, const TinyFad<16,R> & deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() = un.d0() - deux.d0();

  tmp.d1() = un.d1() - deux.d1();

  tmp.d2() = un.d2() - deux.d2();

  tmp.d3() = un.d3() - deux.d3();

  tmp.d4() = un.d4() - deux.d4();

  tmp.d5() = un.d5() - deux.d5();

  tmp.d6() = un.d6() - deux.d6();

  tmp.d7() = un.d7() - deux.d7();

  tmp.d8() = un.d8() - deux.d8();

  tmp.d9() = un.d9() - deux.d9();

  tmp.d10() = un.d10() - deux.d10();

  tmp.d11() = un.d11() - deux.d11();

  tmp.d12() = un.d12() - deux.d12();

  tmp.d13() = un.d13() - deux.d13();

  tmp.d14() = un.d14() - deux.d14();

  tmp.d15() = un.d15() - deux.d15();

  tmp.val() = un.val() - deux.val();

  return tmp;
}

template <class L, class R> inline
TinyFad<16,typename NumericalTraits<L,R>::promote>
operator -(const L & un, const TinyFad<16,R> & deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() -= deux.d0();

  tmp.d1() -= deux.d1();

  tmp.d2() -= deux.d2();

  tmp.d3() -= deux.d3();

  tmp.d4() -= deux.d4();

  tmp.d5() -= deux.d5();

  tmp.d6() -= deux.d6();

  tmp.d7() -= deux.d7();

  tmp.d8() -= deux.d8();

  tmp.d9() -= deux.d9();

  tmp.d10() -= deux.d10();

  tmp.d11() -= deux.d11();

  tmp.d12() -= deux.d12();

  tmp.d13() -= deux.d13();

  tmp.d14() -= deux.d14();

  tmp.d15() -= deux.d15();

  tmp.val() = un - deux.val();

  return tmp;
}

template <class L, class R> inline
TinyFad<16, typename NumericalTraits<L,R>::promote >
operator -(const TinyFad<16,L> & un, const R & deux) {
  typedef typename NumericalTraits<L,R>::promote value_type;

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() = un.d0();

  tmp.d1() = un.d1();

  tmp.d2() = un.d2();

  tmp.d3() = un.d3();

  tmp.d4() = un.d4();

  tmp.d5() = un.d5();

  tmp.d6() = un.d6();

  tmp.d7() = un.d7();

  tmp.d8() = un.d8();

  tmp.d9() = un.d9();

  tmp.d10() = un.d10();

  tmp.d11() = un.d11();

  tmp.d12() = un.d12();

  tmp.d13() = un.d13();

  tmp.d14() = un.d14();

  tmp.d15() = un.d15();

  tmp.val() = un.val() - deux;

  return tmp;
}

template <class L, class R> inline
TinyFad<16, typename NumericalTraits<L,R>::promote >
operator /(const TinyFad<16,L> & un, const TinyFad<16,R> & deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  if (deux.val() == 0.) error("TinyFad & TinyFad::operator /(const TinyFad<16,L> & un, const TinyFad<16,R> & deux), dividing by 0");

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );
  value_type dval = deux.val();

  tmp.d0() = ( un.d0()* deux.val() - un.val() * deux.d0() ) / dval / dval ;

  tmp.d1() = ( un.d1()* deux.val() - un.val() * deux.d1() ) / dval / dval ;

  tmp.d2() = ( un.d2()* deux.val() - un.val() * deux.d2() ) / dval / dval ;

  tmp.d3() = ( un.d3()* deux.val() - un.val() * deux.d3() ) / dval / dval ;

  tmp.d4() = ( un.d4()* deux.val() - un.val() * deux.d4() ) / dval / dval ;

  tmp.d5() = ( un.d5()* deux.val() - un.val() * deux.d5() ) / dval / dval ;

  tmp.d6() = ( un.d6()* deux.val() - un.val() * deux.d6() ) / dval / dval ;

  tmp.d7() = ( un.d7()* deux.val() - un.val() * deux.d7() ) / dval / dval ;

  tmp.d8() = ( un.d8()* deux.val() - un.val() * deux.d8() ) / dval / dval ;

  tmp.d9() = ( un.d9()* deux.val() - un.val() * deux.d9() ) / dval / dval ;

  tmp.d10() = ( un.d10()* deux.val() - un.val() * deux.d10() ) / dval / dval ;

  tmp.d11() = ( un.d11()* deux.val() - un.val() * deux.d11() ) / dval / dval ;

  tmp.d12() = ( un.d12()* deux.val() - un.val() * deux.d12() ) / dval / dval ;

  tmp.d13() = ( un.d13()* deux.val() - un.val() * deux.d13() ) / dval / dval ;

  tmp.d14() = ( un.d14()* deux.val() - un.val() * deux.d14() ) / dval / dval ;

  tmp.d15() = ( un.d15()* deux.val() - un.val() * deux.d15() ) / dval / dval ;

  tmp.val() = un.val() / dval;

  return tmp;
}

template <class L, class R> inline
TinyFad<16, typename NumericalTraits<L,R>::promote >
operator /(const L & un, const TinyFad<16,R> & deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  if (deux.val() == 0.) error("TinyFad & TinyFad::operator /(const L & un, const TinyFad<16,R> & deux), dividing by 0");

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );
  value_type dval = deux.val();

  tmp.d0() = - un * deux.d0()  / dval / dval ;

  tmp.d1() = - un * deux.d1()  / dval / dval ;

  tmp.d2() = - un * deux.d2()  / dval / dval ;

  tmp.d3() = - un * deux.d3()  / dval / dval ;

  tmp.d4() = - un * deux.d4()  / dval / dval ;

  tmp.d5() = - un * deux.d5()  / dval / dval ;

  tmp.d6() = - un * deux.d6()  / dval / dval ;

  tmp.d7() = - un * deux.d7()  / dval / dval ;

  tmp.d8() = - un * deux.d8()  / dval / dval ;

  tmp.d9() = - un * deux.d9()  / dval / dval ;

  tmp.d10() = - un * deux.d10()  / dval / dval ;

  tmp.d11() = - un * deux.d11()  / dval / dval ;

  tmp.d12() = - un * deux.d12()  / dval / dval ;

  tmp.d13() = - un * deux.d13()  / dval / dval ;

  tmp.d14() = - un * deux.d14()  / dval / dval ;

  tmp.d15() = - un * deux.d15()  / dval / dval ;

  tmp.val() = un / dval;

  return tmp;
}

template <class  L, class R> inline
TinyFad<16, typename NumericalTraits<L,R>::promote >
operator /(const TinyFad<16,L> & un, const R & deux) {

  typedef typename NumericalTraits<L,R>::promote value_type;

  if (deux == 0.) error("TinyFad & TinyFad::operator /(const TinyFad<16,L> & un, const R & deux), dividing by 0");

  No_Initialization nothing;
  TinyFad<16,value_type> tmp( nothing );

  tmp.d0() = un.d0()  / deux;

  tmp.d1() = un.d1()  / deux;

  tmp.d2() = un.d2()  / deux;

  tmp.d3() = un.d3()  / deux;

  tmp.d4() = un.d4()  / deux;

  tmp.d5() = un.d5()  / deux;

  tmp.d6() = un.d6()  / deux;

  tmp.d7() = un.d7()  / deux;

  tmp.d8() = un.d8()  / deux;

  tmp.d9() = un.d9()  / deux;

  tmp.d10() = un.d10()  / deux;

  tmp.d11() = un.d11()  / deux;

  tmp.d12() = un.d12()  / deux;

  tmp.d13() = un.d13()  / deux;

  tmp.d14() = un.d14()  / deux;

  tmp.d15() = un.d15()  / deux;

  tmp.val() = un.val() / deux;

   return tmp;
}

template <class T> TinyFad<16,T> exp (const TinyFad<16,T>& in)
{
  TinyFad<16,T> tmp(exp(in.val()));

  tmp.d0() = in.d0()*exp(in.val());
  tmp.d1() = in.d1()*exp(in.val());
  tmp.d2() = in.d2()*exp(in.val());
  tmp.d3() = in.d3()*exp(in.val());
  tmp.d4() = in.d4()*exp(in.val());
  tmp.d5() = in.d5()*exp(in.val());
  tmp.d6() = in.d6()*exp(in.val());
  tmp.d7() = in.d7()*exp(in.val());
  tmp.d8() = in.d8()*exp(in.val());
  tmp.d9() = in.d9()*exp(in.val());
  tmp.d10() = in.d10()*exp(in.val());
  tmp.d11() = in.d11()*exp(in.val());
  tmp.d12() = in.d12()*exp(in.val());
  tmp.d13() = in.d13()*exp(in.val());
  tmp.d14() = in.d14()*exp(in.val());
  tmp.d15() = in.d15()*exp(in.val());

  return tmp;
}

template <class T> TinyFad<16,T> log (const TinyFad<16,T>& in)
{
  if ( in.val() <= 0) error("TinyFad log (const TinyFad& in) : zero or negative value");
  TinyFad<16,T> tmp(log(in.val()));

  tmp.d0() = in.d0() / in.val();
  tmp.d1() = in.d1() / in.val();
  tmp.d2() = in.d2() / in.val();
  tmp.d3() = in.d3() / in.val();
  tmp.d4() = in.d4() / in.val();
  tmp.d5() = in.d5() / in.val();
  tmp.d6() = in.d6() / in.val();
  tmp.d7() = in.d7() / in.val();
  tmp.d8() = in.d8() / in.val();
  tmp.d9() = in.d9() / in.val();
  tmp.d10() = in.d10() / in.val();
  tmp.d11() = in.d11() / in.val();
  tmp.d12() = in.d12() / in.val();
  tmp.d13() = in.d13() / in.val();
  tmp.d14() = in.d14() / in.val();
  tmp.d15() = in.d15() / in.val();

  return tmp;
}

template <class T> TinyFad<16,T> sqrt (const TinyFad<16,T>& in)
{
  if ( in.val() < 0. ) error("TinyFad<16,T> sqrt (const TinyFad& in) : negative value");
  TinyFad<16,T> tmp(sqrt(in.val()));

  bool test=true;
  if ( in.val() == T(0.) ){
    if ( in.d0() != T(0.) ) test = false;

    if ( in.d1() != T(0.) ) test = false;

    if ( in.d2() != T(0.) ) test = false;

    if ( in.d3() != T(0.) ) test = false;

    if ( in.d4() != T(0.) ) test = false;

    if ( in.d5() != T(0.) ) test = false;

    if ( in.d6() != T(0.) ) test = false;

    if ( in.d7() != T(0.) ) test = false;

    if ( in.d8() != T(0.) ) test = false;

    if ( in.d9() != T(0.) ) test = false;

    if ( in.d10() != T(0.) ) test = false;

    if ( in.d11() != T(0.) ) test = false;

    if ( in.d12() != T(0.) ) test = false;

    if ( in.d13() != T(0.) ) test = false;

    if ( in.d14() != T(0.) ) test = false;

    if ( in.d15() != T(0.) ) test = false;

    if ( !test )
      error("TinyFad<16,T> sqrt (const TinyFad& in) : null value");
  }
  else {
    tmp.d0() = in.d0() / sqrt(in.val()) / 2.;
    tmp.d1() = in.d1() / sqrt(in.val()) / 2.;
    tmp.d2() = in.d2() / sqrt(in.val()) / 2.;
    tmp.d3() = in.d3() / sqrt(in.val()) / 2.;
    tmp.d4() = in.d4() / sqrt(in.val()) / 2.;
    tmp.d5() = in.d5() / sqrt(in.val()) / 2.;
    tmp.d6() = in.d6() / sqrt(in.val()) / 2.;
    tmp.d7() = in.d7() / sqrt(in.val()) / 2.;
    tmp.d8() = in.d8() / sqrt(in.val()) / 2.;
    tmp.d9() = in.d9() / sqrt(in.val()) / 2.;
    tmp.d10() = in.d10() / sqrt(in.val()) / 2.;
    tmp.d11() = in.d11() / sqrt(in.val()) / 2.;
    tmp.d12() = in.d12() / sqrt(in.val()) / 2.;
    tmp.d13() = in.d13() / sqrt(in.val()) / 2.;
    tmp.d14() = in.d14() / sqrt(in.val()) / 2.;
    tmp.d15() = in.d15() / sqrt(in.val()) / 2.;
  }

  return tmp;
}

template <class T> TinyFad<16,T> sin (const TinyFad<16,T>& in)
{
  TinyFad<16,T> tmp( sin(in.val()) );

  tmp.d0() = in.d0() * cos( in.val() );
  tmp.d1() = in.d1() * cos( in.val() );
  tmp.d2() = in.d2() * cos( in.val() );
  tmp.d3() = in.d3() * cos( in.val() );
  tmp.d4() = in.d4() * cos( in.val() );
  tmp.d5() = in.d5() * cos( in.val() );
  tmp.d6() = in.d6() * cos( in.val() );
  tmp.d7() = in.d7() * cos( in.val() );
  tmp.d8() = in.d8() * cos( in.val() );
  tmp.d9() = in.d9() * cos( in.val() );
  tmp.d10() = in.d10() * cos( in.val() );
  tmp.d11() = in.d11() * cos( in.val() );
  tmp.d12() = in.d12() * cos( in.val() );
  tmp.d13() = in.d13() * cos( in.val() );
  tmp.d14() = in.d14() * cos( in.val() );
  tmp.d15() = in.d15() * cos( in.val() );

  return tmp;
}

template <class T> TinyFad<16,T> cos (const TinyFad<16,T>& in)
{
  TinyFad<16,T> tmp(cos(in.val()));

  tmp.d0() = - in.d0() * sin( in.val() );
  tmp.d1() = - in.d1() * sin( in.val() );
  tmp.d2() = - in.d2() * sin( in.val() );
  tmp.d3() = - in.d3() * sin( in.val() );
  tmp.d4() = - in.d4() * sin( in.val() );
  tmp.d5() = - in.d5() * sin( in.val() );
  tmp.d6() = - in.d6() * sin( in.val() );
  tmp.d7() = - in.d7() * sin( in.val() );
  tmp.d8() = - in.d8() * sin( in.val() );
  tmp.d9() = - in.d9() * sin( in.val() );
  tmp.d10() = - in.d10() * sin( in.val() );
  tmp.d11() = - in.d11() * sin( in.val() );
  tmp.d12() = - in.d12() * sin( in.val() );
  tmp.d13() = - in.d13() * sin( in.val() );
  tmp.d14() = - in.d14() * sin( in.val() );
  tmp.d15() = - in.d15() * sin( in.val() );

  return tmp;
}

template <class T> TinyFad<16,T> tan (const TinyFad<16,T>& in)
{
  if ( in.val() == 0) error("TinyFad tan (const TinyFad& in) undiefined in 0.");
  TinyFad<16,T> tmp(tan(in.val()));

  T cosinus = cos(in.val());
  tmp.d0() = in.d0() / cosinus / cosinus;
  tmp.d1() = in.d1() / cosinus / cosinus;
  tmp.d2() = in.d2() / cosinus / cosinus;
  tmp.d3() = in.d3() / cosinus / cosinus;
  tmp.d4() = in.d4() / cosinus / cosinus;
  tmp.d5() = in.d5() / cosinus / cosinus;
  tmp.d6() = in.d6() / cosinus / cosinus;
  tmp.d7() = in.d7() / cosinus / cosinus;
  tmp.d8() = in.d8() / cosinus / cosinus;
  tmp.d9() = in.d9() / cosinus / cosinus;
  tmp.d10() = in.d10() / cosinus / cosinus;
  tmp.d11() = in.d11() / cosinus / cosinus;
  tmp.d12() = in.d12() / cosinus / cosinus;
  tmp.d13() = in.d13() / cosinus / cosinus;
  tmp.d14() = in.d14() / cosinus / cosinus;
  tmp.d15() = in.d15() / cosinus / cosinus;

  return tmp;
}

template <class T> TinyFad<16,T> pow (const TinyFad<16,T>& in, double e)
{
  TinyFad<16,T> tmp(pow(in.val(), e));

  tmp.d0() = e*in.d0()*pow(in.val(), e-1);
  tmp.d1() = e*in.d1()*pow(in.val(), e-1);
  tmp.d2() = e*in.d2()*pow(in.val(), e-1);
  tmp.d3() = e*in.d3()*pow(in.val(), e-1);
  tmp.d4() = e*in.d4()*pow(in.val(), e-1);
  tmp.d5() = e*in.d5()*pow(in.val(), e-1);
  tmp.d6() = e*in.d6()*pow(in.val(), e-1);
  tmp.d7() = e*in.d7()*pow(in.val(), e-1);
  tmp.d8() = e*in.d8()*pow(in.val(), e-1);
  tmp.d9() = e*in.d9()*pow(in.val(), e-1);
  tmp.d10() = e*in.d10()*pow(in.val(), e-1);
  tmp.d11() = e*in.d11()*pow(in.val(), e-1);
  tmp.d12() = e*in.d12()*pow(in.val(), e-1);
  tmp.d13() = e*in.d13()*pow(in.val(), e-1);
  tmp.d14() = e*in.d14()*pow(in.val(), e-1);
  tmp.d15() = e*in.d15()*pow(in.val(), e-1);

  return tmp;
}

template <class T> TinyFad<16,T> pow (const TinyFad<16,T>& un, const TinyFad<16,T>& deux)
{
  if (un.val() == 0) error("TinyFad pow (const TinyFad& un, const TinyFad& deux), un = 0. ");
  TinyFad<16,T> tmp(pow(un.val(), deux.val()));

  tmp.d0() = deux.d0() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d0() * pow(un.val(), deux.val()-1);
  tmp.d1() = deux.d1() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d1() * pow(un.val(), deux.val()-1);
  tmp.d2() = deux.d2() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d2() * pow(un.val(), deux.val()-1);
  tmp.d3() = deux.d3() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d3() * pow(un.val(), deux.val()-1);
  tmp.d4() = deux.d4() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d4() * pow(un.val(), deux.val()-1);
  tmp.d5() = deux.d5() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d5() * pow(un.val(), deux.val()-1);
  tmp.d6() = deux.d6() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d6() * pow(un.val(), deux.val()-1);
  tmp.d7() = deux.d7() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d7() * pow(un.val(), deux.val()-1);
  tmp.d8() = deux.d8() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d8() * pow(un.val(), deux.val()-1);
  tmp.d9() = deux.d9() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d9() * pow(un.val(), deux.val()-1);
  tmp.d10() = deux.d10() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d10() * pow(un.val(), deux.val()-1);
  tmp.d11() = deux.d11() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d11() * pow(un.val(), deux.val()-1);
  tmp.d12() = deux.d12() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d12() * pow(un.val(), deux.val()-1);
  tmp.d13() = deux.d13() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d13() * pow(un.val(), deux.val()-1);
  tmp.d14() = deux.d14() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d14() * pow(un.val(), deux.val()-1);
  tmp.d15() = deux.d15() * log(un.val()) * pow(un.val(), deux.val())
	    + deux.val() * un.d15() * pow(un.val(), deux.val()-1);

  return tmp;
}

template <class T> TinyFad<16,T> pow (const TinyFad<16,T>& in, const int e)
{
  TinyFad<16,T> tmp( std::pow((double)in.val(), (double)e) );

  tmp.d0() = e*in.d0()*std::pow((double)in.val(), (double)e-1);
  tmp.d1() = e*in.d1()*std::pow((double)in.val(), (double)e-1);
  tmp.d2() = e*in.d2()*std::pow((double)in.val(), (double)e-1);
  tmp.d3() = e*in.d3()*std::pow((double)in.val(), (double)e-1);
  tmp.d4() = e*in.d4()*std::pow((double)in.val(), (double)e-1);
  tmp.d5() = e*in.d5()*std::pow((double)in.val(), (double)e-1);
  tmp.d6() = e*in.d6()*std::pow((double)in.val(), (double)e-1);
  tmp.d7() = e*in.d7()*std::pow((double)in.val(), (double)e-1);
  tmp.d8() = e*in.d8()*std::pow((double)in.val(), (double)e-1);
  tmp.d9() = e*in.d9()*std::pow((double)in.val(), (double)e-1);
  tmp.d10() = e*in.d10()*std::pow((double)in.val(), (double)e-1);
  tmp.d11() = e*in.d11()*std::pow((double)in.val(), (double)e-1);
  tmp.d12() = e*in.d12()*std::pow((double)in.val(), (double)e-1);
  tmp.d13() = e*in.d13()*std::pow((double)in.val(), (double)e-1);
  tmp.d14() = e*in.d14()*std::pow((double)in.val(), (double)e-1);
  tmp.d15() = e*in.d15()*std::pow((double)in.val(), (double)e-1);

  return tmp;
}

template <class T> TinyFad<16,T> abs (const TinyFad<16,T>& in)
{
  int sign = in.val() > 0? 1:0;

  if (sign) return in;
  else return (-in);
}

template <class T> ostream& operator << (ostream& os, const TinyFad<16,T>& a)
{
  os.setf(ios::fixed,ios::scientific);
  os.width(12);
  os << a.val() << "  [";

  os.width(12);
  os << a.d0();
  os.width(12);
  os << a.d1();
  os.width(12);
  os << a.d2();
  os.width(12);
  os << a.d3();
  os.width(12);
  os << a.d4();
  os.width(12);
  os << a.d5();
  os.width(12);
  os << a.d6();
  os.width(12);
  os << a.d7();
  os.width(12);
  os << a.d8();
  os.width(12);
  os << a.d9();
  os.width(12);
  os << a.d10();
  os.width(12);
  os << a.d11();
  os.width(12);
  os << a.d12();
  os.width(12);
  os << a.d13();
  os.width(12);
  os << a.d14();
  os.width(12);
  os << a.d15();

  os << "]";

  return os;
}


#endif
