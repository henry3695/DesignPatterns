
#include <fstream>
#include <string>
int file()
{
	string usr_name("henry3695");
	int num_tries(5);
	int num_right(6);

	/*
	如果文件不存在，便会有一个文件被产生出来并打开供输出使用。
	如果指定的文件已经存在，这个文件会被打开用于输出，而文件中原有的数据会被丢弃
	*/
	//ofstream outfile("seq_data.txt");

	/*
	以追加模式打开seq_data.txt
	新数据会被加到文件末尾
	*/
	ofstream outfile("seq_data.txt", ios_base::app);
	if (!outfile)
		cerr << "open fail!\n";
	else
		outfile << usr_name << ' '
		<< num_tries << ' '
		<< num_right << endl;

	outfile.close();


	//以读取模式打开文件
	ifstream infile("seq_data.txt");
	if (!infile)
	{
		cerr << "open fail\n";
	}
	else
	{
		usr_name = "";
		num_tries = 0;
		num_right = 0;
		while (infile >> usr_name)
		{
			infile >> num_tries >> num_right;

			cout << usr_name << "," << num_tries << "," << num_right << endl;
		}
	}

	//想要同时读写同一个文件
	// Open a new file for input/output operations discarding any current
	// content in the file (assumes a length of zero on opening)
	std::fstream myFile("test.txt", std::ios::in | std::ios::out | std::ios::trunc);

	// Add the characters "Hello World" to the file
	myFile << "Hello World";

	// Seek to 6 characters from the beginning of the file
	/*
	static constexpr _Seekdir beg = _Seekbeg;  从文件头开始计算
	static constexpr _Seekdir cur = _Seekcur;  从当前文件指针开始计算
	static constexpr _Seekdir end = _Seekend;  从文件末尾指针开始计算
	*/
	myFile.seekg(6, std::ios::beg);

	// Read the next 5 characters from the file into a buffer
	string buffer;
	myFile >> buffer;

	// Output the contents read from the file and close it 
	std::cout << buffer << std::endl;
	//World

	myFile.close();

	return 0;
}