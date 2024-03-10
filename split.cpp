#include <iostream>
#include <vector>
using namespace std;
const vector<string> split(const string& s, const char& c)
{
	string buff{""};
	vector<string> v;
	
	for(auto n:s)
	{
		if(n != c) 
        {
            buff+=n;
        }
        else
		{
            if(n == c && buff != "") 
            {
                v.push_back(buff);
                buff = ""; 
            }
        }
	}
	if(buff != "") 
    v.push_back(buff);
	
	return v;
}

int main()
{
	string str{"the quick brown fox jumps over the lazy dog"};
	vector<string> v{split(str, ' ')};
	for(auto n:v) 
    {
        cout << n << endl;
    }
	
	return 0;
}