#include <iostream>
#include <iomanip>
#include <string>
#include <memory>

#include <boost/pool/object_pool.hpp>
#include <boost/scoped_ptr.hpp>


int allocate_counter{0};
int deallocate_counter{0};


class A
{
public:
    A(): A(99,"default"){ //delegate constructor
      std::cout << "delegate constructor \n";
    }

    A(int _value, const std::string& _tag):
        m_value{_value},
        m_tag{_tag}
    {
        std::cout << "object allocated on " << m_tag <<
            " has the value set to: " << m_value << "\n";

        allocate_counter++;
    }

    A(const A& _o) : m_value(_o.m_value),
                    m_tag(_o.m_tag) {
      std::cout << "copy constructor,  move failed!\n";

      allocate_counter++;
    }

    A(A&& _o) noexcept : m_value(std::move(_o.m_value)),
                        m_tag(std::move(_o.m_tag)) {
      std::cout << "move constructor: " << m_tag <<
          " has the value set to: " << m_value << "\n";

      allocate_counter++;
    }

    ~A()
    {
        std::cout << "object allocated on " << m_tag << " and value is: " << m_value <<
            " is going down.\n";

      deallocate_counter++;
    }

    void* operator new(std::size_t _size)throw(std::bad_alloc){
      std::cout << "operator new on A: " << _size << " bytes" << std::endl;
      return ::operator new(_size);
    }

    void operator delete(void* _addr){
      std::cout << "operator delete on A: " << _addr << std::endl;
      return ::operator delete(_addr);
    }

    int value() const
    {
        return m_value;
    }

    std::string tag() const
    {
        return m_tag;
    }
private:
    int m_value;
    std::string m_tag;
};

class B: public A{
  public:
 ~B() { } // destructor prevents implicit move ctor A::(A&&)
};

class C:public A{
  public:
  C() = default;
  ~C() { }          // destructor would prevent implicit move ctor B::(B&&)
  C(C&&) = default; // force a move ctor anyway
};

A f(A a)
{
    return a;
}

void allocFunc(){
  boost::scoped_ptr<boost::object_pool<A>> pool(new boost::object_pool<A>);

  //memory pool
  auto poolElement = pool->malloc();
  ::new (poolElement)A(222, "pool");

  auto poolElement2 = pool->malloc();
  ::new (poolElement2)A(333, "pool");

  //static instance
  static A staticObj{1, "static"};

  //stack instance
  A stackObject{2, "stack"};

  //heap method 1
  A* heapObject1 = (A*)::operator new(sizeof(A));
  ::new (heapObject1)A(11, "heap");

  //heap method 2
  auto heapObject2 = new A(22,"heap");

  //heap method 3
  auto sharedObject = std::make_shared<A>(33,"heap"); //one allocation
  auto sharedObject2 = sharedObject;
  auto anotherSharedObject = std::shared_ptr<A>(new A(333,"heap")); //two allocations (new and smartptr)

  //heap method 4
  auto uniqueObject = std::make_unique<A>(77,"heap");
  auto uniqueObject2 = std::move(uniqueObject);


  std::weak_ptr<A> weakObject = sharedObject;
  auto sharedObject3 = weakObject.lock();

  //pool objects
  pool->destroy(poolElement);
  pool->destroy(poolElement2);


  //destroy method 1
  heapObject1->~A();
  ::operator delete(heapObject1);

  //destroy method 2
  delete heapObject2;
}

void moveFunc(){

  std::cout << "Trying to move A\n";
  A a1 = f(A()); // move-construct from rvalue temporary
  A a2 = std::move(a1); // move-construct from xvalue


  std::cout << "Trying to move B\n";
  B b1;
  std::cout << "Before move, b1.tag = " << std::quoted(b1.tag()) << "\n";
  B b2 = std::move(b1); // calls implicit move ctor
  std::cout << "After move, b1.tag = " << std::quoted(b1.tag()) << "\n";

  std::cout << "Trying to move C\n";
  C c1;
  std::cout << "Before move, c1.tag = " << std::quoted(c1.tag()) << "\n";
  C c2 = std::move(c1); // calls the copy constructor
  std::cout << "After move, c1.tag = " << std::quoted(c1.tag()) << "\n";
}

void atexit_handler(){
  std::cout << "Allocate counter: " << allocate_counter << std::endl;
  std::cout << "Deallocate counter: " << deallocate_counter << std::endl;
}

A global_a(123,"global");

const int result = std::atexit(atexit_handler);

int main()
{
  std::cout << "Test allocate/deallocate \n";
  allocFunc();

  std::cout << "Test moving constructor\n";
  moveFunc();

  return EXIT_SUCCESS;
}
