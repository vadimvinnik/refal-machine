#include <algorithm>
#include <initializer_list>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <assert.h>
#include "refcount_ptr.h"

class Expression;
class Term;
class TermEnumerator;
class ExpressionNode;

typedef refcount_ptr<Term const> PCTerm;
typedef refcount_ptr<Expression const> PCExpression;
typedef std::list<PCExpression> ExpressionList;

class Expression {
public:
  enum Direction {
    LeftToRight,
    RightToLeft
  };

protected:
  class TermEnumeratorBase;
  typedef refcount_ptr<TermEnumeratorBase> PTermEnumeratorBase;

  class TermEnumeratorBase {
  public:
    virtual ~TermEnumeratorBase() {}
    virtual bool isEqualTo(const TermEnumeratorBase& x) const = 0;
    bool operator==(const TermEnumeratorBase& x) const { return isEqualTo(x); }
    bool operator!=(const TermEnumeratorBase& x) const { return !isEqualTo(x); }

    virtual void move(Direction direction) = 0;
    TermEnumeratorBase& operator++() { move(LeftToRight); return *this; }
    TermEnumeratorBase& operator--() { move(RightToLeft); return *this; }

    virtual PCTerm current() const = 0;
    operator PCTerm() const { return current(); }
    PCTerm operator->() const { return current(); }
  };
  
public:
  class TermEnumerator : public TermEnumeratorBase {
  public:
    TermEnumerator(PTermEnumeratorBase pDelegee) : m_pDelegee(pDelegee) {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto xAsTermEnumerator = dynamic_cast<TermEnumerator const*>(&x);
      return nullptr != xAsTermEnumerator && m_pDelegee->isEqualTo(*xAsTermEnumerator->m_pDelegee);
    }

    virtual void move(Direction direction) { m_pDelegee->move(direction); }
    virtual PCTerm current() const { return m_pDelegee->current(); }

  private:
    PTermEnumeratorBase m_pDelegee;
  };

  virtual ~Expression() {}
  virtual std::string toString() const = 0;
  virtual bool isEmpty() const = 0;
  virtual TermEnumerator front() const { return TermEnumerator(frontImpl()); }
  virtual TermEnumerator back() const { return TermEnumerator(backImpl()); }

protected:
  virtual PTermEnumeratorBase frontImpl() const = 0;
  virtual PTermEnumeratorBase backImpl() const = 0;
};

class ExpressionNode : public Expression {
public:
  virtual int termsCount() const = 0;
};

/*
class Empty : public ExpressionNode {
public:
  class EmptyEnumerator : public Expression::TermEnumeratorBase {
  public:
    virtual bool isEqualTo(const TermEnumeratorBase& x) const { return nullptr != dynamic_cast<EmptyEnumerator const*>(&x); }
    virtual void move(Direction direction) { assert(false); }
    virtual PCTerm current() const { assert(false); return nullptr; }
  };

  virtual std::string toString() const { return std::string(); }
  virtual bool isEmpty() const { return true; }
  virtual int termsCount() const { return 0; }

protected:
  virtual PTermEnumeratorBase frontImpl() const { return PTermEnumeratorBase(new EmptyEnumerator()); }
  virtual PTermEnumeratorBase backImpl() const { return PTermEnumeratorBase(new EmptyEnumerator()); }
};
*/

class Term : public ExpressionNode {
public:
  virtual bool isEmpty() const { return false; }
  virtual int termsCount() const { return 1; }

protected:
  class TermSelfEnumerator : public Expression::TermEnumeratorBase {
  public:
    TermSelfEnumerator(PCTerm pTarget, bool bFinished) : m_pTarget(pTarget), m_bFinished(bFinished) {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto te = dynamic_cast<TermSelfEnumerator const*>(&x);
      return nullptr != te && te->m_pTarget == m_pTarget && te->m_bFinished == m_bFinished;
    }

    virtual void move(Direction direction) {
      static const bool bFinish[] = {true, false};
      auto const bNewFinished = bFinish[direction];
      assert(bNewFinished != m_bFinished);
      m_bFinished = bNewFinished;
    }

    virtual PCTerm current() const { return m_bFinished ? PCTerm(nullptr) : m_pTarget; }

  private:
    PCTerm m_pTarget;
    bool m_bFinished;
  };

  virtual PTermEnumeratorBase frontImpl() const { return createEnumerator(false); }
  virtual PTermEnumeratorBase backImpl() const { return createEnumerator(true); }

private:
  PTermEnumeratorBase createEnumerator(bool bFinished) const {
    return PTermEnumeratorBase(new TermSelfEnumerator(PCTerm(this), bFinished));
  }
};

class Symbol : public Term {
public:
  Symbol(char cSymbol) : m_cSymbol(cSymbol) {}
  virtual std::string toString() const { return std::string(1, m_cSymbol); }

private:
  char m_cSymbol;
};

class Parenthesized : public Term {
};

class Literal : public ExpressionNode {
public:
  Literal(std::string sSymbols) : m_sSymbols(sSymbols) {}
  virtual std::string toString() const { return m_sSymbols; }
  virtual bool isEmpty() const { return m_sSymbols.empty(); }
  virtual int termsCount() const { return m_sSymbols.length(); }

protected:
  class SymbolEnumerator : public ExpressionNode::TermEnumeratorBase {
  public:
    SymbolEnumerator(std::string::const_iterator iPosition) : m_iPosition(iPosition) {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto se = dynamic_cast<SymbolEnumerator const*>(&x);
      return nullptr != se && se->m_iPosition == m_iPosition;
    }

    virtual void move(Direction direction) {
      static const int offsets[] = {+1, -1};
      m_iPosition += offsets[direction];
    }

    virtual PCTerm current() const { return PCTerm(new Symbol(*m_iPosition)); }

  private:
    std::string::const_iterator m_iPosition;
  };

  virtual PTermEnumeratorBase frontImpl() const { return PTermEnumeratorBase(new SymbolEnumerator(m_sSymbols.cbegin())); }
  virtual PTermEnumeratorBase backImpl() const { return PTermEnumeratorBase(new SymbolEnumerator(m_sSymbols.cend())); }

private:
  std::string m_sSymbols;
};

class Concatenation : public Expression {
public:
  Concatenation(std::initializer_list<PCExpression> lComponents) : m_lComponents(lComponents.begin(), lComponents.end()) {}

  virtual std::string toString() const {
    ostringstream s;
    std::for_each(m_lComponents.cbegin(), m_lComponents.cend(), [&s](PCExpression const& pe) { s << pe->toString(); });
    return s.str();
  }

  virtual bool isEmpty() const {
    return std::all_of(m_lComponents.cbegin(), m_lComponents.cend(), isPtrToEmpty);
  }

protected:
  /*
  Only works for non-empty concatenations of non-empty components.
  TODO: generalize it to handle both
  */
  class ConcatenationTermEnumerator : public TermEnumeratorBase {
  public:
    ConcatenationTermEnumerator(
      ExpressionList::const_iterator const& iCurrentComponent,
      ExpressionList::const_iterator const& iComponentsEnd
    ) :
      m_iCurrentComponent(iCurrentComponent), 
      m_iComponentsEnd(iComponentsEnd),
      m_iCurrentTerm(nullptr)
    {
      findNextTerm();
    }

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto te = dynamic_cast<ConcatenationTermEnumerator const*>(&x);
      return
        nullptr != te &&
	te->m_iCurrentComponent == m_iCurrentComponent &&
	te->m_iComponentsEnd == m_iComponentsEnd &&
	te->isAtEnd() == isAtEnd() &&
	(isAtEnd() || te->m_iCurrentTerm == m_iCurrentTerm);
    }

    /*
    for now, only works fromlefft to right.
    TODO: reverse direction as well
    */
    virtual void move(Direction direction) {
      assert(!isAtEnd());
      ++m_iCurrentTerm;
      if (m_iCurrentTerm == (*m_iCurrentComponent)->back())
      {
        ++m_iCurrentComponent;
        findNextTerm();
      }
    }

    virtual PCTerm current() const { return isAtEnd() ? PCTerm() : m_iCurrentTerm; }

  private:
    bool isAtEnd() const { return m_iCurrentComponent == m_iComponentsEnd; }

    void findNextTerm() {
      m_iCurrentComponent = find_if_not(m_iCurrentComponent, m_iComponentsEnd, isPtrToEmpty);

      if (!isAtEnd())
      {
        m_iCurrentTerm = (*m_iCurrentComponent)->front();
      }
    }

    ExpressionList::const_iterator m_iCurrentComponent;
    ExpressionList::const_iterator m_iComponentsEnd;
    TermEnumerator m_iCurrentTerm;
  };

  virtual PTermEnumeratorBase frontImpl() const { return PTermEnumeratorBase(new ConcatenationTermEnumerator(m_lComponents.cbegin(), m_lComponents.cend())); }
  virtual PTermEnumeratorBase backImpl() const { return PTermEnumeratorBase(new ConcatenationTermEnumerator(m_lComponents.cend(), m_lComponents.cend())); }

private:
  static bool isPtrToEmpty(PCExpression const& pe) { return pe->isEmpty(); }
  ExpressionList m_lComponents;
};

class SubExpression : public Expression {
public:

private:
  refcount_ptr<Expression> m_pTarget;
  
};

