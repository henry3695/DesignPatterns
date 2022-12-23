
string int32_to_string(int32_t n)
{
	string s;
	char sz[4] = { 0 };
	sz[0] = uint32_t(n) & 0x000000FF;
	sz[1] = (uint32_t(n) & 0x0000FF00) >> 8;
	sz[2] = (uint32_t(n) & 0x00FF0000) >> 16;
	sz[3] = (uint32_t(n) & 0xFF000000) >> 24;

	s.assign(&sz[0], 4);
	return s;
}


int32_t string_to_int32(string s)
{
	if (s.size() != sizeof(int32_t))
		return 0xFFFFFFFF;

	const char *p = s.c_str();

	uint32_t t = 0;
	t |= (0xFF000000 & (p[3] << 24));
	t |= (0x00FF0000 & (p[2] << 16));
	t |= (0x0000FF00 & (p[1] << 8));
	t |= (0x000000FF & p[0]);

	return (int32_t)(t);
}
std::string encode(const google::protobuf::Message& message)
{
	const int32_t nHead = 8;
	std::string result;
	result.resize(nHead);//len|checkSum|nameLen|nameLen+1 字符串|len-(nameLen+1)-12 字符串

	const std::string& typeName = message.GetTypeName();
	int32_t nameLen = static_cast<int32_t>(typeName.size() + 1);
	string s = int32_to_string(nameLen);
	result.append(s.c_str(), s.size());
	result.append(typeName.c_str(), nameLen);

	bool succeed = message.AppendToString(&result);

	if (succeed)
	{
		const  char* begin = result.c_str() + nHead;
		int32_t checkSum = 0;
		for (int i = nHead; i < (result.size() - nHead); i++)
		{
			checkSum += (int32_t)begin[i];
		}
		s = int32_to_string(result.size());
		std::copy(s.begin(), s.end(), result.begin());

		s = int32_to_string(checkSum);
		std::copy(s.begin(), s.end(), result.begin() + 4);
	}
	else
	{
		result.clear();
	}

	return result;
}

bool decode(const std::string& buf)
{
	const int nHead = 8;
	if (buf.size()<12)
	{
		return false;
	}
	string s;
	int32_t nLen = string_to_int32(s.assign(buf.c_str(), 4));
	int32_t checkSum = string_to_int32(s.assign(buf.c_str() + 4, 4));

	int32_t checkSum1 = 0;
	const  char* begin = buf.c_str() + nHead;
	for (int i = nHead; i<(buf.size() - nHead); i++)
	{
		checkSum1 += (int32_t)begin[i];
	}
	if (checkSum != checkSum1)
	{
		return false;
	}
	int32_t nameLen = string_to_int32(s.assign(buf.c_str() + 8, 4));

	if (buf.size() <= (12 + nameLen))
	{
		return false;
	}

	std::string typeName = s.assign(buf.c_str() + 12, nameLen);
	string buffer(buf.c_str() + 12 + nameLen, nLen - (12 + nameLen));
	emit  sigCallBackUdpData(typeName, buffer);
}



