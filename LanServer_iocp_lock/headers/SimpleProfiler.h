#pragma once

extern CLog logger;

class SimpleProfiler
{
public:


	struct Profile {
		char name[100] = {0,};
		LARGE_INTEGER start;
		__int64 sum = 0;
		__int64 max = 0;
		__int64 min = 0x7FFFFFFFFFFFFFFF;
		__int64 callCnt = 0;
	};

	LARGE_INTEGER freq;

public:

	SimpleProfiler() {

		tlsIndex = TlsAlloc();
		if(tlsIndex == TLS_OUT_OF_INDEXES){
			// loging

			logger(L"profiler.txt", LOG_GROUP::LOG_ERROR, L"profiler tls alloc error, %s, %d", __FILEW__, __LINE__);

			CDump::crash();
		}

		QueryPerformanceFrequency(&freq);
		freq.QuadPart /= 1000000;
	}
	
	void profileBegin(const char name[100]) {
				
		Profile* profile = getProfileData();

		int idx = findIdx(name);

		if(idx == -1){

			for(idx = 0; idx < 50; idx++){
				if(profile[idx].callCnt == 0){
					break;
				}
			}

			profile[idx].callCnt = 1;
			strcpy_s(profile[idx].name, name);

		} else {

			profile[idx].callCnt += 1;

		}
		

		QueryPerformanceCounter(&profile[idx].start);
	}

	void profileEnd(const char name[100]) {
		
		LARGE_INTEGER endTime;
		QueryPerformanceCounter(&endTime);

		Profile* profile = getProfileData();

		for (int idx = 0; idx < 50; ++idx) {
		
			if(strcmp(name, profile[idx].name) != 0){
				continue;
			}

			__int64 time = endTime.QuadPart - profile[idx].start.QuadPart;

			if (profile[idx].max < time) {
				profile[idx].max = time;
			}

			if (profile[idx].min > time) {
				profile[idx].min = time;
			}

			profile[idx].sum += time;

			break;
		}

	}

	void printToFile() {

		FILE* outFile;
		fopen_s(&outFile, "profiler.txt", "w");



		for(int profileCnt = 0; profileCnt < allocIndex; ++ profileCnt){
		
			Profile* profile = _profile[profileCnt];

			fprintf_s(outFile, "%20s\n", "(100ns)");
			fprintf_s(outFile, "%20s | %15s | %15s | %15s | %15s\n", "Name", "Average", "Min", "Max", "Call");
		
			for (int idx = 0; idx < 50; ++idx) {
			
				if (profile[idx].callCnt > 0) {
					profile[idx].sum = profile[idx].sum - profile[idx].max - profile[idx].min;
					profile[idx].callCnt -= 2;
					fprintf_s(outFile, "%20s | %14.4lf | %15I64d | %15I64d | %15I64d \n",
						profile[idx].name,
						((double)profile[idx].sum / (double)profile[idx].callCnt),
						profile[idx].min,
						profile[idx].max,
						profile[idx].callCnt);
				}
			}

			fprintf_s(outFile, "\n");
		}

		for(int profileCnt = 1; profileCnt < allocIndex; ++ profileCnt){
		
			Profile* profile = _profile[profileCnt];
		
			for (int idx = 0; idx < 50; ++idx) {
				for(int firstIdx = 0; firstIdx < 50; ++ firstIdx){
					if (profile[idx].callCnt > 0) {

						if(strcmp(profile[idx].name, _profile[0][firstIdx].name) == 0){

							_profile[0][firstIdx].callCnt += profile[idx].callCnt;
							_profile[0][firstIdx].sum += profile[idx].sum;

						}
					}
				}
			}
		}

		Profile* profile = _profile[0];

		fprintf_s(outFile, "%20s\n", "(100ns)");
		fprintf_s(outFile, "%20s | %15s | %15s | %15s | %15s\n", "Name", "Average", "Min", "Max", "Call");
		
		for (int idx = 0; idx < 50; ++idx) {
			
			if (profile[idx].callCnt > 0) {
				profile[idx].sum = profile[idx].sum - profile[idx].max - profile[idx].min;
				profile[idx].callCnt -= 2;
				fprintf_s(outFile, "%20s | %14.4lf | %15I64d | %15I64d | %15I64d \n",
					profile[idx].name,
					((double)profile[idx].sum / (double)profile[idx].callCnt),
					profile[idx].min,
					profile[idx].max,
					profile[idx].callCnt);
			}
		}

		fprintf_s(outFile, "\n");
		

		fclose(outFile);

	}
	

private:

	unsigned int tlsIndex = 0;
	unsigned short allocIndex = 0;

	Profile _profile[20][50]; 

	Profile* getProfileData(){

		Profile* ptr = (Profile*)TlsGetValue(tlsIndex);

		if(ptr == nullptr){

			int idx  = InterlockedIncrement16((SHORT*)&allocIndex) - 1;
			ptr = _profile[idx];
			TlsSetValue(tlsIndex, ptr);

		}

		return ptr;

	}
	
	int findIdx(const char* name) {
	
		Profile* profile = getProfileData();

		int idx = 0;

		for(; idx < 50 ; ++idx){
						
			if(strcmp(name, profile[idx].name) != 0){
				continue;
			}

			return idx;

		}
		
		return -1;
	}

	void killProcess() {
		int* kill = nullptr;
		*kill = 1;
	}


};



//__declspec(thread) SimpleProfiler sp;
