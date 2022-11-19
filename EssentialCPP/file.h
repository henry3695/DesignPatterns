
#include <fstream>
#include <string>
int file()
{
	string usr_name("henry3695");
	int num_tries(5);
	int num_right(6);

	/*
	����ļ������ڣ������һ���ļ��������������򿪹����ʹ�á�
	���ָ�����ļ��Ѿ����ڣ�����ļ��ᱻ��������������ļ���ԭ�е����ݻᱻ����
	*/
	//ofstream outfile("seq_data.txt");

	/*
	��׷��ģʽ��seq_data.txt
	�����ݻᱻ�ӵ��ļ�ĩβ
	*/
	ofstream outfile("seq_data.txt", ios_base::app);
	if (!outfile)
		cerr << "open fail!\n";
	else
		outfile << usr_name << ' '
		<< num_tries << ' '
		<< num_right << endl;

	outfile.close();


	//�Զ�ȡģʽ���ļ�
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

	//��Ҫͬʱ��дͬһ���ļ�
	// Open a new file for input/output operations discarding any current
	// content in the file (assumes a length of zero on opening)
	std::fstream myFile("test.txt", std::ios::in | std::ios::out | std::ios::trunc);

	// Add the characters "Hello World" to the file
	myFile << "Hello World";

	// Seek to 6 characters from the beginning of the file
	/*
	static constexpr _Seekdir beg = _Seekbeg;  ���ļ�ͷ��ʼ����
	static constexpr _Seekdir cur = _Seekcur;  �ӵ�ǰ�ļ�ָ�뿪ʼ����
	static constexpr _Seekdir end = _Seekend;  ���ļ�ĩβָ�뿪ʼ����
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