#include <iostream>
using namespace std;


class Operateor
{

protected:
    int m_nFirst;
    int m_nSecond;
public:
    virtual int GetResult()=0;
};

class OperatorAdd:public Operateor
{
    public:
        OperatorAdd(int a,int b)
        {
            m_nFirst=a;
            m_nSecond=b;
        }

        int GetResult(){
            return m_nFirst+m_nSecond;
        }
};
class OperatorSub:public Operateor
{
    public:
        OperatorSub(int a,int b)
        {
            m_nFirst=a;
            m_nSecond=b;
        }

        int GetResult(){
            return m_nFirst-m_nSecond;
        }
};

class OperatorMul:public Operateor
{
    public:
        OperatorMul(int a,int b)
        {
            m_nFirst=a;
            m_nSecond=b;
        }

        int GetResult(){
            return m_nFirst*m_nSecond;
        }
};

class OperatorDiv:public Operateor
{
    public:
        OperatorDiv(int a,int b)
        {
            m_nFirst=a;
            m_nSecond=b;
        }

        int GetResult(){
            if(m_nSecond==0)
                return 0x7FFFFFFF;
            return m_nFirst/m_nSecond;
        }
};

class  Context
{
private:
    Operateor* m_Operateor;
public:
     Context(int a,int b,char op);
    ~ Context();

    int GetResult();
};

 Context:: Context(int a,int b,char op)
{
    m_Operateor = NULL;
     switch (op)
    {
    case '+':
        m_Operateor = new OperatorAdd(a,b);
        break;
    
    case '-':
        m_Operateor = new OperatorSub(a,b);
        break;
    case '*':
        m_Operateor = new OperatorMul(a,b);
        break;
    case '/':
        m_Operateor = new OperatorDiv(a,b);
        break;
    default:
        break;
    }
}

 Context::~ Context()
{
    if(m_Operateor)
    {
        delete m_Operateor;
    }
}

int Context::GetResult(){
    if(m_Operateor)
    {
        return m_Operateor->GetResult();
    }

    return  0;   
}



int main()
{
    cout<<"工厂与策略模式"<<endl;


    Context calc(5,10,'+');
    cout<<calc.GetResult()<<endl;


    Context* p = new Context(5,10,'-');
    cout<<p->GetResult()<<endl;
    delete p;

    p = new Context(5,10,'*');
    cout<<p->GetResult()<<endl;
    delete p;
    
    return 0;
}