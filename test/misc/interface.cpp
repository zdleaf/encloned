#include <iostream>
#include <memory>

using namespace std;

class C;

class B{
    private:
        std::shared_ptr<C> c;
      
    public:
        int ib = 2;
        B(){
            cout << "B created" << endl;
        }

        void setC(std::shared_ptr<C> c){
            this->c = c;
        }

        int getIB(){ return ib; }
        int getIC();

};

class C{
    private:
        std::shared_ptr<B> b;

    public:
        int ic = 3;

        C(){
            cout << "C created" << endl;
        }

        void setB(std::shared_ptr<B> b){
            this->b = b;
        }

        int getIC(){ return ic; }
        int getIB();

};

class A{
    private:
        std::shared_ptr<B> b;
        std::shared_ptr<C> c;

        //std::shared_ptr<Interface> i;

    public:
        A(){
            b = std::make_shared<B>();
            c = std::make_shared<C>();
            //i = std::make_shared<Interface>(b, c);
            cout << "A created" << endl;
            b->setC(c);
            c->setB(b);
        }

        std::shared_ptr<B> getBPtr(){
            return b;
        }

        std::shared_ptr<C> getCPtr(){
            return c;
        }
};

int B::getIC(){ return c->getIC(); }

int C::getIB(){ return b->getIB(); }


int main(){
    A a;
    cout << "get ic from B: " << a.getBPtr()->getIC() << endl;
    cout << "get ib from C: " << a.getCPtr()->getIB() << endl;
}