#include <iostream>
#include <windows.h>
#include <wininet.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>

#pragma comment(lib, "wininet.lib")

using namespace std;


class THttpReader
{
public:
	THttpReader(wstring lpszAgent, wstring lpszServerName, bool bUseSsl)
	{
		m_agent = lpszAgent;
		m_url = lpszServerName;
		m_bUseSsl = bUseSsl;
		m_rawData.clear();

		Init();
	}

	THttpReader()
	{
		m_agent.clear();
		m_url.clear();
		m_bUseSsl = false;
		m_rawData.clear();
	}

	~THttpReader()
	{
		InternetCloseHandle(m_hInternet);
		InternetCloseHandle(m_hConnect);
		InternetCloseHandle(m_hRequest);
		m_url.clear();
		m_rawData.clear();
		m_agent.clear();
	}

	bool OpenConnection(wstring lpszAgent, wstring lpszServerName, bool bUseSsl)
	{
		m_agent = lpszAgent;
		m_url = lpszServerName;
		m_bUseSsl = bUseSsl;
		m_rawData = TEXT("");

		Init();
	}

	bool Get(wstring lpszObjectName, wstring lpszReferrer)
	{
		bool result = false;

		m_hRequest = HttpOpenRequest(m_hConnect, TEXT("GET"), lpszObjectName.c_str(), NULL, lpszReferrer.c_str(), NULL, INTERNET_FLAG_KEEP_CONNECTION, 1u);

		if (m_hRequest != NULL)
		{
			bool bSend = HttpSendRequest(m_hRequest, NULL, 0, NULL, 0);

			if (bSend)
			{	
				wstringstream sstrm;
				while (true)
				{
					const size_t sz = 512;
					char szData[sz];
					DWORD dwBytesRead;

					bool bRead = InternetReadFile(m_hRequest, szData, sizeof(szData) - 1, &dwBytesRead);

					if (!bRead || dwBytesRead == 0)
					{
						break;
					}
					szData[dwBytesRead] = L'\0';

					wchar_t szBuffer[sz];
					size_t len;
					mbstowcs_s(&len, szBuffer, szData, sz);

					sstrm << szBuffer;
					result = true;
				}
				m_rawData = sstrm.str();
			}
		}
		return result;
	}

	inline bool IsInitialized()
	{
		return m_bInitialized;
	}

	inline wstring GetRawData()
	{
		return m_rawData.c_str();
	}

private:
	void Init()
	{
		m_hInternet = InternetOpen(m_agent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

		if (m_hInternet == NULL)
		{
			m_bInitialized = false;
			throw exception("Couldn't initialize WinInet.");
			return;
		}

		INTERNET_PORT port = m_bUseSsl ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
		m_hConnect = InternetConnect(m_hInternet, m_url.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1u);

		if (m_hConnect == NULL)
		{
			m_bInitialized = false;
			throw exception("Couldn't connect to url.");
			return;
		}
		m_bInitialized = true;
	}
private:
	HINTERNET m_hInternet;

	HINTERNET m_hConnect;

	HINTERNET m_hRequest;

	bool m_bUseSsl;

	wstring m_url;

	wstring m_agent;

	wstring m_rawData;

	bool m_bInitialized;
};

class TFileStorage
{
public:
	TFileStorage(wstring fileName)
	{
		this->fileName = fileName;
	}

	TFileStorage()
	{
	}

	~TFileStorage()
	{
		fin.close();
		fout.close();
	}

	void Open(wstring fileName)
	{
		this->fileName = fileName;
	}

	void Write(wstring str)
	{
		fout.open(fileName, ios::app);
		if (fout.is_open())
		{
			fout << str;
		}
		fout.close();
	}

	void Clear()
	{
		wofstream clear_stream(fileName);
		if (clear_stream.is_open())
		{
			clear_stream.close();
		}
	}

	vector<wstring> ReadAllLines()
	{
		wstring msg;
		vector<wstring> result;

		fin.open(fileName);
		if (fin.is_open())
		{
			fin.imbue(locale("en_US.UTF-8"));
			while (!fin.eof())
			{
				msg.clear();
				getline(fin, msg);
				result.push_back(msg);
			}
		}
		fin.close();
		return result;
	}

private:
	wstring fileName;

	wofstream fout;

	wifstream fin;
};


vector<wstring> Split(wstring rawData, wchar_t delim = L'\n')
{
	vector<wstring> result;
	size_t len = rawData.length();
	wstring temp;
	for (size_t i = 0; i < len; i++)
	{
		if (rawData[i] == delim)
		{
			result.push_back(temp);
			temp.clear();
			continue;
		}
		temp += rawData[i];
	}
	if(!temp.empty()) result.push_back(temp);
	return result;
}

wstring GetCurrentDatetime()
{
	wstring result;
	time_t t = time(NULL);
	tm now;

	localtime_s(&now, &t);

	wchar_t year[21] = L"";
	wchar_t month[11] = L"";
	wchar_t day[3] = L"";
	wchar_t hour[3] = L"";
	wchar_t min[3] = L"";

	_itow_s(now.tm_year + 1900, year, 10);
	_itow_s(now.tm_mday, day, 10);
	_itow_s(now.tm_hour, hour, 10);
	_itow_s(now.tm_min, min, 10);
	
	if (now.tm_min < 10)
	{
		wchar_t temp[3] = L"";
		_itow_s(now.tm_min, temp, 10);

		wcscpy_s(min, L"0");
		wcscat_s(min, temp);
	}
	else
	{
		_itow_s(now.tm_min, min, 10);
	}

	if (now.tm_mon < 10)
	{
		wchar_t temp[3] = L"";
		_itow_s(now.tm_mon + 1, temp, 10);

		wcscpy_s(month, L"0");
		wcscat_s(month, temp);
	}
	else
	{
		_itow_s(now.tm_mon + 1, month, 10);
	}

	result = year + wstring(L"-") + month + wstring(L"-") + day + wstring(L", ") + hour + wstring(L":") + min;
	return result;
}

wstring GetCurrentDatetime(TFileStorage& storage)
{
	return storage.ReadAllLines()[0];
}

void SaveAllDataToStorage(TFileStorage& storage, wstring data)
{
	storage.Clear();
	storage.Write(L"#Rate from: " + GetCurrentDatetime() + L"\n");
	storage.Write(data);
}

void DivideCurrencyRatesByMultipliers(vector<wstring>& currencyRates, vector<wstring> currencyMultipliers)
{
	for (size_t i = 0; i < currencyRates.size(); i++)
	{
		wstring currencyMultiplier = currencyMultipliers[i];
		wstring currencyRate = currencyRates[i];
		replace(currencyRate.begin(), currencyRate.end(), L',', L'.');

		double fRate = _wtof(currencyRate.c_str());
		double fMultiplier = _wtof(currencyMultiplier.c_str());

		currencyRates[i] = to_wstring(fRate / fMultiplier);
	}
}

vector<wstring> LoadCurrencyData(TFileStorage& storage, int index)
{
	vector<wstring> data = storage.ReadAllLines();
	vector<wstring> result;

	for (size_t i = 0; i < data.size(); i++)
	{
		if (data[i][0] != L'#' && !data[i].empty())
		{
			vector<wstring> splittedStr = Split(data[i], L':');
			if(index < splittedStr.size())
				result.push_back(splittedStr[index]);
		}
	}
	return result;
}

vector<wstring> ParseCurrencyData(vector<wstring> splittedData, int index)
{
	vector<wstring> result;
	int startIndex = 0, endIndex = 0;

	for (size_t i = 0; i < splittedData.size(); i++)
	{
		if (splittedData[i].find(L"<td>") != wstring::npos)
		{
			startIndex = i + index;
			break;
		}
	}
	for (int i = splittedData.size() - 1; i >= 0; i--)
	{
		if (splittedData[i].find(L"<td>") != wstring::npos)
		{
			endIndex = i;
			break;
		}
	}
	for (size_t i = startIndex; i <= endIndex; i += 7)
	{
		result.push_back(splittedData[i]);
	}
	for (size_t i = 0; i < result.size(); i++)
	{
		wstring currencyResultName;
		for (size_t j = 0; j < result[i].size(); j++)
		{
			if (result[i][j] == L'>')
			{
				do
				{
					j++;

					if (result[i][j] == L'<') break;

					currencyResultName += result[i][j];
				} while (result[i][j] != L'<');

				if (result[i][j] == L'<') break;
			}
		}
		result[i] = currencyResultName;
		currencyResultName.clear();
	}
	return result;
}

vector<wstring> currencyNames;
vector<wstring> currencyFullNames;
vector<wstring> currencyIDs;
vector<wstring> currencyMultipliers;
vector<wstring> currencyRates;

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	wcout.imbue(locale("Russian.UTF8"));
	wcin.imbue(locale("Russian.UTF8"));

	TFileStorage storage(L"currency_rates.txt");

	int choise;
	wcout << L"Enter your choise:\n" << L"1) Display currency rate\n" << L"2) Open currency converter\n" << L">> ";
	cin >> choise;

	if (InternetCheckConnection(L"http://www.cbr.ru", FLAG_ICC_FORCE_CONNECTION, 0))
	{
		THttpReader reader(L"Reader-Agent", L"www.cbr.ru", false);
		if (reader.IsInitialized() && reader.Get(L"currency_base/daily", L""))
		{
			auto splittedRawData = Split(reader.GetRawData());

			currencyIDs = ParseCurrencyData(splittedRawData, 0);
			currencyNames = ParseCurrencyData(splittedRawData, 1);
			currencyMultipliers = ParseCurrencyData(splittedRawData, 2);
			currencyFullNames = ParseCurrencyData(splittedRawData, 3);
			currencyRates = ParseCurrencyData(splittedRawData, 4);
			DivideCurrencyRatesByMultipliers(currencyRates, currencyMultipliers);

			wstring dataToSave;
			for (size_t i = 0; i < currencyNames.size(); i++)
			{
				dataToSave += currencyIDs[i] + L':' + currencyNames[i] + L':' + L'1' + L':' + currencyFullNames[i] + L':' + currencyRates[i] + L'\n';
			}
			SaveAllDataToStorage(storage, dataToSave);
		}
	}
	currencyIDs = LoadCurrencyData(storage, 0);
	currencyNames = LoadCurrencyData(storage, 1);
	currencyMultipliers = LoadCurrencyData(storage, 2);
	currencyFullNames = LoadCurrencyData(storage, 3);
	currencyRates = LoadCurrencyData(storage, 4);
	DivideCurrencyRatesByMultipliers(currencyRates, currencyMultipliers);

	if (choise == 1)
	{
		wcout << GetCurrentDatetime(storage) << endl;
		for (size_t i = 0; i < currencyNames.size(); i++)
		{
			wcout << currencyNames[i] << L":" << currencyFullNames[i] << L":" << currencyRates[i] << endl;
		}
	}
	else if (choise == 2)
	{
		double x = 1.0;
		wstring cr1, cr2;

		wcout << L"Input first currency (USD/JPY etc.): ";
		wcin >> cr1;
		wcout << L"Input second currency (USD/JPY etc.): ";
		wcin >> cr2;
		wcout << L"Input how many first currency units should be converted to second currency: ";
		wcin >> x;

		transform(cr1.begin(), cr1.end(), cr1.begin(), ::toupper);
		transform(cr2.begin(), cr2.end(), cr2.begin(), ::toupper);

		double firstRate = 0.0, secondRate = 0.0;
		for (size_t i = 0; i < currencyNames.size(); i++)
		{
			if (currencyNames[i] == cr1)
			{
				firstRate = _wtof(currencyRates[i].c_str());
			}
			if (currencyNames[i] == cr2)
			{
				secondRate = _wtof(currencyRates[i].c_str());
			}
		}
		wcout << L"Converting " << x << " " << cr1 << L" to " << cr2 << L" is " << x * (firstRate / secondRate);
	}
	system("pause");

}



