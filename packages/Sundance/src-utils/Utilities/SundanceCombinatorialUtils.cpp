#include "SundanceCombinatorialUtils.hpp"
#include "SundanceExceptions.hpp"
#include "SundanceTabs.hpp"
#include <algorithm>
#include <iterator>
#include <iostream>


namespace SundanceUtils
{
  using namespace Teuchos;

  Array<Array<int> > partitionInteger(int n)
  {
    typedef Array<int> Aint;
    typedef Array<Array<int> > AAint;
    static Array<Array<Array<int> > > rtn =
      tuple<AAint>(
        tuple<Aint>(tuple(1)), 
        tuple<Aint>(tuple(2), tuple(1, 1)),
        tuple<Aint>(tuple(3), tuple(2, 1), tuple(1, 1, 1)),
        tuple<Aint>(tuple(4), tuple(3, 1), tuple(2, 2), tuple(2, 1, 1), tuple(1,1,1,1)));
    TEST_FOR_EXCEPTION(n<1 || n>4, RuntimeError, 
      "case n=" << n << " not implemented in partitionInteger()");
    return rtn[n-1];
  }



  


  Array<Array<Array<int> > > compositions(int n)
  {
    Array<Array<Array<int> > > q(n);

    Array<Array<int> > x = partitionInteger(n);

    Array<Array<int> > p;
    for (unsigned int m=0; m<x.size(); m++)
      {
        Array<int> tmp;
        Array<int> y = x[m];
        std::sort(y.begin(), y.end());
        tmp.resize(y.size());
        std::copy(y.begin(), y.end(), tmp.begin());
        p.append(tmp);
        while (std::next_permutation(y.begin(), y.end())) 
          { 
            tmp.resize(y.size());
            std::copy(y.begin(), y.end(), tmp.begin());
            p.append(tmp);
          }
      }

    for (unsigned int i=0; i<p.size(); i++)
      {
        q[p[i].size()-1].append(p[i]);
      }

    return q;
  }

  Array<Array<int> > nonNegCompositions(unsigned int n, unsigned int J)
  {
    /* find all partitions */
    Array<Array<int> > parts = partitionInteger(n);
    
    /* find the partitions into J or fewer terms */
    Array<Array<int> > jParts;
    for (unsigned int i=0; i<parts.size(); i++)
      {
        if (parts[i].size() <= J) jParts.append(parts[i]);
      }

    /* Pad with zeros until all remaining partitions have size=J */
    for (unsigned int i=0; i<jParts.size(); i++)
      {
        for (unsigned int j=jParts[i].size(); j<J; j++)
          {
            jParts[i].append(0);
          }
      }

    /* form all permutations */
    Array<Array<int> > all;
    for (unsigned int i=0; i<jParts.size(); i++)
      {
        Array<int> tmp;
        Array<int> y = jParts[i];
        std::sort(y.begin(), y.end());
        tmp.resize(y.size());
        std::copy(y.begin(), y.end(), tmp.begin());
        all.append(tmp);
        while (std::next_permutation(y.begin(), y.end())) 
          { 
            tmp.resize(y.size());
            std::copy(y.begin(), y.end(), tmp.begin());
            all.append(tmp);
          }
      }

    return all;
  }


  
  Set<Pair<MultiSet<int> > >
  loadPartitions(int x, int n, 
                 const MultiSet<int>& left, 
                 const MultiSet<int>& right)
  {
    Set<Pair<MultiSet<int> > > rtn;
    for (int i=0; i<n; i++)
      {
        MultiSet<int> L = left;
        MultiSet<int> R = right;

        for (int j=0; j<n; j++) 
          {
            if (j < i) L.put(x);
            else R.put(x);
          }
        rtn.put(Pair<MultiSet<int> >(L, R));
        rtn.put(Pair<MultiSet<int> >(R, L));
      }
    return rtn;
  }


  Set<Pair<MultiSet<int> > >
  binaryPartition(const MultiSet<int>& m)
  {
    Set<Pair<MultiSet<int> > > tmp1;


    Map<int, int> counts = countMap(m);
    
    for (Map<int, int>::const_iterator 
           i=counts.begin(); i!=counts.end(); i++)
      {
        int x = i->first;
        int n = i->second;
        if (tmp1.size()==0U)
          {
            MultiSet<int> L;
            MultiSet<int> R;
            tmp1 = loadPartitions(x, n, L, R);
          }
        else
          {
            Set<Pair<MultiSet<int> > > tmp2;
            for (Set<Pair<MultiSet<int> > >::const_iterator
                   j=tmp1.begin(); j!=tmp1.end(); j++)
              {
                MultiSet<int> L = j->first();
                MultiSet<int> R = j->second();
                Set<Pair<MultiSet<int> > > t 
                  = loadPartitions(x, n, L, R);
                tmp2.merge(t);
              }
            tmp1 = tmp2;
          }
      }

#ifdef BLAH
    Set<SortedPair<MultiSet<int> > > rtn;
    for (Set<Pair<MultiSet<int> > >::const_iterator
           j=tmp1.begin(); j!=tmp1.end(); j++)
      {
        rtn.put(SortedPair<MultiSet<int> >(j->first(), j->second()));
      }
#endif

    
    return tmp1;
  }



  Set<MultiSet<MultiSet<int> > >
  multisetPartitions(const MultiSet<int>& m)
  {
    Set<MultiSet<MultiSet<int> > > rtn;
    unsigned int mSize = m.size();
    if (m.size()==0U) return rtn;
    if (m.size()==1U) return makeSet(makeMultiSet(m));
    rtn.put(makeMultiSet(m));

    Set<Pair<MultiSet<int> > > 
      twoParts = binaryPartition(m);

    for (Set<Pair<MultiSet<int> > >::const_iterator 
           i=twoParts.begin(); i!=twoParts.end(); i++)
      {
        MultiSet<int> L = i->first();
        MultiSet<int> R = i->second();
        if (L.size() == mSize || R.size() == mSize) continue;
        if (L.size() > 0U && R.size() > 0U) rtn.put(makeMultiSet(L, R));

        Array<MultiSet<MultiSet<int> > > lParts;
        Array<MultiSet<MultiSet<int> > > rParts;
        if (L.size() > 0U)
          {
            lParts = multisetPartitions(L).elements();
          }
        if (R.size() > 0U)
          {
            rParts = multisetPartitions(R).elements();
          }
        for (unsigned int j=0; j<lParts.size(); j++)
          {
            for (unsigned int k=0; k<rParts.size(); k++)
              {
                const MultiSet<MultiSet<int> >& a = lParts[j];
                const MultiSet<MultiSet<int> >& b = rParts[k];
                MultiSet<MultiSet<int> > combined;
                for (MultiSet<MultiSet<int> >::const_iterator 
                       x=a.begin(); x!=a.end(); x++)
                  {
                    combined.put(*x);
                  }
                for (MultiSet<MultiSet<int> >::const_iterator 
                       x=b.begin(); x!=b.end(); x++)
                  {
                    combined.put(*x);
                  }
                rtn.put(combined);
              }
          }
      }
    return rtn;
  }


  Map<int, int> countMap(const MultiSet<int>& m)
  {
    Map<int, int> rtn;
    for (MultiSet<int>::const_iterator i=m.begin(); i!=m.end(); i++)
      {
        if (rtn.containsKey(*i)) rtn[*i]++;
        else rtn.put(*i, 1);
      }
    return rtn;
  }


  Array<Array<int> > indexCombinations(const Array<int>& s)
  {

    Array<int> D(s.size(), 0);
    Array<int> C(s.size(), -1);

    int p=1;
    for (unsigned int k=1; k<=s.size(); k++)
      {
        D[s.size()-k] = p;
        p = p*s[s.size() - k];
      }
    Array<Array<int> > rtn(p);

    for (int i=0; i<p; i++)
      {
        for (unsigned int j=0; j<s.size(); j++)
          {
            if ( (i % D[j])==0 )
              {
                C[j] = (C[j]+1) % s[j];
              }
          }
        rtn[i] = C;
      }
    return rtn;
  }


  Array<Array<Array<int> > > binnings(const MultiSet<int>& mu, int n)
  {
    int N = mu.size();
    Array<Array<int> > c = compositions(N)[n-1];
    Array<Array<Array<int> > > rtn;

    for (unsigned int i=0; i<c.size(); i++)
      {
        Array<Array<Array<int> > > a = indexArrangements(mu, c[i]);
        for (unsigned int j=0; j<a.size(); j++)
          {
            rtn.append(a[j]);
          }
      }
    return rtn;
  }



  Array<Array<Array<int> > > indexArrangements(const MultiSet<int>& mu,
                                               const Array<int> & k) 
  {
    int nBins = k.size();
    
    int M = 0;
    for (int i=0; i<nBins; i++)
      {
        M += k[i];
      }

    Array<int> I;
    for (MultiSet<int>::const_iterator iter=mu.begin(); iter!=mu.end(); iter++)
      {
        I.append(*iter);
      }

    Array<Array<Array<int> > > rtn;
    
    do
      {
        Array<Array<int> > bins(nBins);
        int count = 0;
        for (int i=0; i<nBins; i++)
          {
            for (int j=0; j<k[i]; j++)
              {
                bins[i].append(I[count++]);
              }
          }
        rtn.append(bins);
      }
    while (std::next_permutation(I.begin(), I.end()));
    return rtn;
    
  }


  Set<MultiSet<int> > multisetSubsets(const MultiSet<int>& x)
  {
    /* We'll generate the subsets by traversing them in bitwise order.
     * For a multiset having N elements, there are up to 2^N subsets each
     * of which can be described by a N-bit number with the i-th
     * bit indicating whether the i-th element is in the subset. Note that
     * with a multiset, repetitions can occur so we need to record the
     * results in a Set object to eliminate duplicates. 
     */

    /* Make an indexable array of the elements. This will be convenient
     * because we'll need to access the i-th element after reading
     * the i-th bit. */
    Array<int> elements = x.elements();

    /* Compute the maximum number of subsets. This number will be reached
    * only in the case of no repetitions. */
    int n = elements.size();
    int maxNumSubsets = pow2(n);
    
    Set<MultiSet<int> > rtn;
    
    
    /* Loop over subsets in bitwise order. We start the count at 1 
       to avoid including the empty subset */
    for (int i=1; i<maxNumSubsets; i++)
      {
        Array<int> bits = bitsOfAnInteger(i, n);
        MultiSet<int> ms;
        for (int j=0; j<n; j++) 
          {
            if (bits[j] == 1) ms.put(elements[j]);
          }
        rtn.put(ms);
      }
    return rtn;
  }


  Array<Array<MultiSet<int> > > multisetCompositions(unsigned int s,
                                                     const MultiSet<int>& x)
  {
    Set<MultiSet<MultiSet<int> > > parts = multisetPartitions(x);

    Array<Array<MultiSet<int> > > rtn;

    typedef Set<MultiSet<MultiSet<int> > >::const_iterator iter;
    for (iter i=parts.begin(); i!=parts.end(); i++)
      {
        if (i->size()!=s) continue;
        Array<MultiSet<int> > y = i->elements();
        Array<MultiSet<int> > tmp(y.size());
        std::sort(y.begin(), y.end());
        std::copy(y.begin(), y.end(), tmp.begin());
        rtn.append(tmp);
        while (std::next_permutation(y.begin(), y.end())) 
          { 
            tmp.resize(y.size());
            std::copy(y.begin(), y.end(), tmp.begin());
            rtn.append(tmp);
          }
      }
    return rtn;
  }

  Array<Array<int> > distinctIndexTuples(int m, int n)
  {
    Array<Array<int> > rtn;
    Array<int> cur(m);
    for (int i=0; i<m; i++) cur[i] = i;
    if (m==1) 
      {
        for (int i=0; i<n; i++) rtn.append(tuple(i));
        return rtn;
      }

    while(cur[0] < (n-m)+1)
      {
        rtn.append(cur);
        for (int i=(m-1); i>=0; i--)
          {
            if (cur[i] < (n-1)) 
              {
                cur[i]++;
                bool overflow = false;
                for (int j=i+1; j<m; j++) 
                  {
                    cur[j] = cur[j-1]+1;
                    if (cur[j] >= n) overflow = true;
                  }
                if (overflow) continue;
                else break;
              }
          }
      }
    return rtn;
  }
  
  Array<int> bitsOfAnInteger(int x, int n)
  {
    TEST_FOR_EXCEPTION(x >= pow2(n), InternalError,
                       "Invalid input to bitsOfAnIteger");
                     
    Array<int> rtn(n);
    
    int r = x;
    for (int b=n-1; b>=0; b--)
      {
        rtn[b] = r/pow2(b);
        r = r - rtn[b]*pow2(b);
      }
    return rtn;
  }





  int pow2(int n)
  {
    static Array<int> p2(1,1);

    if ((unsigned int) n >= p2.size())
      {
        int oldN = p2.size(); 
        for (int i=oldN; i<=n; i++) p2.push_back(p2[i-1]*2);
      }
    
    return p2[n];
  }
}



	





