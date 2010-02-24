#include "stdafx.h"
#include "..\main.h"

void ExtensionFromFilename(char *szfilename, char *szextension)
	{
	int begin;

	int len = lstrlen(szfilename);

	for (begin=len;begin>=0;begin--)
		{
		if (szfilename[begin] == '.')
			{
			begin++;
			break;
			}
		}

	if (begin <= 0)
		{
		szextension[0] = '\0';
		}

	lstrcpy(szextension, &szfilename[begin]);
	}

void TitleFromFilename(char *szfilename, char *sztitle)
	{
	int begin, end;

	int len = lstrlen(szfilename);

	for (begin=len;begin>=0;begin--)
		{
		if (szfilename[begin] == '\\')
			{
			begin++;
			break;
			}
		}

	for (end=len;end>=0;end--)
		{
		if (szfilename[end] == '.')
			{
			break;
			}
		}

	if (end == 0)
		{
		end = len-1;
		}

	char *szT = &szfilename[begin];
	int count = end-begin;

	while (count--) {*sztitle++ = *szT++;}
	*sztitle = '\0';
	}

void PathFromFilename(char *szfilename, char *szpath)
	{
	int end;

	int len = lstrlen(szfilename);
	// find the last '\' in the filename
	for (end=len;end>=0;end--)
		{
		if (szfilename[end] == '\\')
			{
			break;
			}
		}
		
	if (end == 0)
		{
		end = len-1;
		}

	// copy from the start of the string to the end (or last '\')
	char *szT = szfilename;
	int count = end+1;

	while (count--) {*szpath++ = *szT++;}
	*szpath = '\0';
	}
	
void TitleAndPathFromFilename(char *szfilename, char *szpath)
	{
	int end;

	int len = lstrlen(szfilename);
	// find the last '.' in the filename
	for (end=len;end>=0;end--)
		{
		if (szfilename[end] == '.')
			{
			break;
			}
		}
		
	if (end == 0)
		{
		end = len;
		}

	// copy from the start of the string to the end (or last '\')
	char *szT = szfilename;
	int count = end;

	while (count--) {*szpath++ = *szT++;}
	*szpath = '\0';
	}

BOOL RawReadFromFile(char *szfilename, int *psize, char **pszout)
	{
	HANDLE hFile = CreateFile(szfilename,
		GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		{
		return fFalse;
		//ShowError("The file could not be opened.");
		//return;
		}

	*psize = GetFileSize(hFile, NULL);

	*pszout = new char[*psize + 2];

	DWORD read;

	BOOL fFoo = ReadFile(hFile, *pszout, *psize, &read, NULL);

	(*pszout)[*psize] = '\0';
	(*pszout)[*psize+1] = '\0'; // In case this is a unicode file, end it with a null character properly

	fFoo = CloseHandle(hFile);

	return fTrue;
	}

BiffWriter::BiffWriter(IStream *pistream, HCRYPTHASH hcrypthash, HCRYPTKEY hcryptkey)
	{
	m_pistream = pistream;
	m_hcrypthash = hcrypthash;
	m_hcryptkey = hcryptkey;
	}

HRESULT BiffWriter::WriteBytes(const void *pv,unsigned long count,unsigned long *foo)
	{
	if (m_hcrypthash)
		{
		CryptHashData(m_hcrypthash, (BYTE *)pv, count, 0);
		}

	return m_pistream->Write(pv, count, foo);
	}

HRESULT BiffWriter::WriteRecordSize(int size)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;

	hr = m_pistream->Write(&size, sizeof(size), &writ);

	return hr;
	}

HRESULT BiffWriter::WriteInt(int id, int value)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;

	if(FAILED(WriteRecordSize(sizeof(int)*2)))
		return hr;

	if(FAILED(hr = WriteBytes(&id, sizeof(int), &writ)))
		return hr;

	hr = WriteBytes(&value, sizeof(int), &writ);

	return hr;
	}

HRESULT BiffWriter::WriteString(int id, char *szvalue)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;
	int len;

	len = lstrlen(szvalue);

	if(FAILED(WriteRecordSize(sizeof(int)*2 + len)))
		return hr;

	if(FAILED(hr = WriteBytes(&id, sizeof(int), &writ)))
		return hr;

	if(FAILED(hr = WriteBytes(&len, sizeof(int), &writ)))
		return hr;

	hr = WriteBytes(szvalue, len, &writ);

	return hr;
	}

HRESULT BiffWriter::WriteWideString(int id, WCHAR *wzvalue)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;
	int len;

	len = lstrlenW(wzvalue) * sizeof(WCHAR);

	if(FAILED(WriteRecordSize(sizeof(int)*2 + len)))
		return hr;

	if(FAILED(hr = WriteBytes(&id, sizeof(int), &writ)))
		return hr;

	if(FAILED(hr = WriteBytes(&len, sizeof(int), &writ)))
		return hr;

	hr = WriteBytes(wzvalue, len, &writ);

	return hr;
	}

HRESULT BiffWriter::WriteBool(int id, BOOL fvalue)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;

	if(FAILED(WriteRecordSize(sizeof(int)*2)))
		return hr;

	if(FAILED(hr = WriteBytes(&id, sizeof(int), &writ)))
		return hr;

	hr = WriteBytes(&fvalue, sizeof(BOOL), &writ);

	return hr;
	}

HRESULT BiffWriter::WriteFloat(int id, float value)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;

	if(FAILED(WriteRecordSize(sizeof(int) + sizeof(float))))
		return hr;

	if(FAILED(hr = WriteBytes(&id, sizeof(int), &writ)))
		return hr;

	hr = WriteBytes(&value, sizeof(float), &writ);

	return hr;
	}

HRESULT BiffWriter::WriteStruct(int id, void *pvalue, int size)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;

	if(FAILED(WriteRecordSize(sizeof(int) + size)))
		return hr;

	if(FAILED(hr = WriteBytes(&id, sizeof(int), &writ)))
		return hr;

	hr = WriteBytes(pvalue, size, &writ);

	return hr;
	}

HRESULT BiffWriter::WriteTag(int id)
	{
	ULONG writ = 0;
	HRESULT hr = S_OK;

	if(FAILED(WriteRecordSize(sizeof(int))))
		return hr;

	hr = WriteBytes(&id, sizeof(int), &writ);

	return hr;
	}

BiffReader::BiffReader(IStream *pistream, ILoadable *piloadable, void *ppassdata, int version, HCRYPTHASH hcrypthash, HCRYPTKEY hcryptkey)
	{
	m_pistream = pistream;
	m_piloadable = piloadable;
	m_pdata = ppassdata;
	m_version = version;

	m_bytesinrecordremaining = 0;

	m_hcrypthash = hcrypthash;
	m_hcryptkey = hcryptkey;
	}

HRESULT BiffReader::ReadBytes(void *pv,unsigned long count,unsigned long *foo)
	{
	HRESULT hr;

	hr = m_pistream->Read(pv, count, foo);

	if (m_hcrypthash)
		{
		CryptHashData(m_hcrypthash, (BYTE *)pv, count, 0);
		}

	return hr;
	}

HRESULT BiffReader::GetIntNoHash(void *pvalue)
	{
	ULONG read = 0;
	HRESULT hr = S_OK;

	m_bytesinrecordremaining -= sizeof(int);

	return m_pistream->Read(pvalue, sizeof(int), &read);
	}

HRESULT BiffReader::GetInt(void *pvalue)
	{
	ULONG read = 0;
	HRESULT hr = S_OK;

	m_bytesinrecordremaining -= sizeof(int);

	return ReadBytes(pvalue, sizeof(int), &read);

	//return m_pistream->Read(pvalue, sizeof(int), &read);
	}

HRESULT BiffReader::GetString(char *szvalue)
	{
	ULONG read = 0;
	HRESULT hr = S_OK;
	int len;

	//if(FAILED(hr = m_pistream->Read(&len, sizeof(int), &read)))
	if(FAILED(hr = ReadBytes(&len, sizeof(int), &read)))
		return hr;

	m_bytesinrecordremaining -= len+sizeof(int);

	//hr = m_pistream->Read(szvalue, len, &read);
	hr = ReadBytes(szvalue, len, &read);
	szvalue[len] = 0;
	return hr;
	}

HRESULT BiffReader::GetWideString(WCHAR *wzvalue)
	{
	ULONG read = 0;
	HRESULT hr = S_OK;
	int len;

	//if(FAILED(hr = m_pistream->Read(&len, sizeof(int), &read)))
	if(FAILED(hr = ReadBytes(&len, sizeof(int), &read)))
		return hr;

	m_bytesinrecordremaining -= len+sizeof(int);

	//hr = m_pistream->Read(wzvalue, len, &read);
	hr = ReadBytes(wzvalue, len, &read);
	wzvalue[len/sizeof(WCHAR)] = 0;
	return hr;
	}

HRESULT BiffReader::GetFloat(float *pvalue)
	{
	ULONG read = 0;
	HRESULT hr = S_OK;

	m_bytesinrecordremaining -= sizeof(float);

	//return m_pistream->Read(pvalue, sizeof(float), &read);
	return ReadBytes(pvalue, sizeof(float), &read);
	}

HRESULT BiffReader::GetBool(BOOL *pfvalue)
	{
	ULONG read = 0;
	HRESULT hr = S_OK;

	m_bytesinrecordremaining -= sizeof(BOOL);

	//return m_pistream->Read(pfvalue, sizeof(BOOL), &read);
	return ReadBytes(pfvalue, sizeof(BOOL), &read);
	}

HRESULT BiffReader::GetStruct(void *pvalue, int size)
	{
	ULONG read = 0;
	HRESULT hr = S_OK;

	m_bytesinrecordremaining -= size;

	//return m_pistream->Read(pvalue, size, &read);
	return ReadBytes(pvalue, size, &read);
	}

HRESULT BiffReader::Load()
	{
	int tag = 0;
	HRESULT hr;
	BOOL fContinue;

	while (tag != FID(ENDB))
		{
		if (m_version > 30)
			{
			GetIntNoHash(&m_bytesinrecordremaining);
			}

		hr = GetInt(&tag);

		if (hr == S_OK)
			{
			fContinue = m_piloadable->LoadToken(tag, this);
			}

		if (!fContinue)
			{
			return E_FAIL;
			}

		if (m_version > 30)
			{
			if (m_bytesinrecordremaining > 0)
				{
				//LARGE_INTEGER li;
				//li.QuadPart = m_bytesinrecordremaining;
				BYTE *szT;
				szT = new BYTE[m_bytesinrecordremaining];
				GetStruct(szT, m_bytesinrecordremaining);
				delete szT;
				//m_pistream->Seek(li, STREAM_SEEK_CUR, NULL);
				}
			}
		}

	return S_OK;
	}

FastIStorage::FastIStorage()
	{
	m_wzName = NULL;
	m_cref = 0;
	}

FastIStorage::~FastIStorage()
	{
	int i;

	for (i=0;i<m_vstg.Size();i++)
		{
		m_vstg.ElementAt(i)->Release();
		}

	for (i=0;i<m_vstm.Size();i++)
		{
		m_vstm.ElementAt(i)->Release();
		}

	SAFE_DELETE(m_wzName);
	}

long __stdcall FastIStorage::QueryInterface(const struct _GUID &,void ** )
	{
	return S_OK;
	}

unsigned long __stdcall FastIStorage::AddRef(void)
	{
	m_cref++;

	return S_OK;
	}

unsigned long __stdcall FastIStorage::Release(void)
	{
	m_cref--;

	if (m_cref == 0)
		{
		delete this;
		}

	return S_OK;
	}

long __stdcall FastIStorage::CreateStream(const OLECHAR *wzName,unsigned long,unsigned long,unsigned long,struct IStream **ppstm)
	{
	FastIStream *pfs = new FastIStream();
	pfs->AddRef(); // AddRef once for us, and once for the caller
	pfs->AddRef();
	pfs->m_wzName = new WCHAR[lstrlenW(wzName)+1];
	WideStrCopy((WCHAR *)wzName, pfs->m_wzName);

	*ppstm = pfs;

	m_vstm.AddElement(pfs);

	return S_OK;
	}

long __stdcall FastIStorage::OpenStream(const OLECHAR *,void *,unsigned long,unsigned long,struct IStream ** )
	{
	return S_OK;
	}

long __stdcall FastIStorage::CreateStorage(const OLECHAR *wzName,unsigned long,unsigned long,unsigned long,struct IStorage **ppstg)
	{
	FastIStorage *pfs = new FastIStorage();
	pfs->AddRef(); // AddRef once for us, and once for the caller
	pfs->AddRef();
	pfs->m_wzName = new WCHAR[lstrlenW(wzName)+1];
	WideStrCopy((WCHAR *)wzName, pfs->m_wzName);

	*ppstg = pfs;

	m_vstg.AddElement(pfs);

	return S_OK;
	}

long __stdcall FastIStorage::OpenStorage(const OLECHAR *,struct IStorage *,unsigned long,SNB ,unsigned long,struct IStorage ** )
	{
	return S_OK;
	}

long __stdcall FastIStorage::CopyTo(unsigned long,const struct _GUID *,SNB ,struct IStorage *pstgNew)
	{
	int i;
	HRESULT hr;
	IStorage *pstgT;
	IStream *pstmT;

	for (i=0;i<m_vstg.Size();i++)
		{
		FastIStorage *pstgCur = m_vstg.ElementAt(i);
		if(SUCCEEDED(hr = pstgNew->CreateStorage(pstgCur->m_wzName, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, &pstgT)))
			{
			pstgCur->CopyTo(0,NULL,NULL,pstgT);
			pstgT->Release();
			}
		}

	for (i=0;i<m_vstm.Size();i++)
		{
		FastIStream *pstmCur = m_vstm.ElementAt(i);
		if(SUCCEEDED(hr = pstgNew->CreateStream(pstmCur->m_wzName, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, &pstmT)))
			{
			ULONG writ;
			//pstmCur->CopyTo(0,NULL,NULL,pstmT);
			pstmT->Write(pstmCur->m_rg, pstmCur->m_cSize, &writ);
			pstmT->Release();
			}
		}

	return S_OK;
	}

long __stdcall FastIStorage::MoveElementTo(const OLECHAR *,struct IStorage *,const OLECHAR *,unsigned long)
	{
	return S_OK;
	}

long __stdcall FastIStorage::Commit(unsigned long)
	{
	return S_OK;
	}

long __stdcall FastIStorage::Revert(void)
	{
	return S_OK;
	}

long __stdcall FastIStorage::EnumElements(unsigned long,void *,unsigned long,struct IEnumSTATSTG ** )
	{
	return S_OK;
	}

long __stdcall FastIStorage::DestroyElement(const OLECHAR *)
	{
	return S_OK;
	}

long __stdcall FastIStorage::RenameElement(const OLECHAR *,const OLECHAR *)
	{
	return S_OK;
	}

long __stdcall FastIStorage::SetElementTimes(const OLECHAR *,const struct _FILETIME *,const struct _FILETIME *,const struct _FILETIME *)
	{
	return S_OK;
	}

long __stdcall FastIStorage::SetClass(const struct _GUID &)
	{
	return S_OK;
	}

long __stdcall FastIStorage::SetStateBits(unsigned long,unsigned long)
	{
	return S_OK;
	}

long __stdcall FastIStorage::Stat(struct tagSTATSTG *,unsigned long)
	{
	return S_OK;
	}

FastIStream::FastIStream()
	{
	m_cref = 0;

	m_cMax = 0;
	m_cSeek = 0;
	m_cSize = 0;
	m_rg = NULL;

	m_wzName = NULL;
	}

FastIStream::~FastIStream()
	{
	free(m_rg);
	SAFE_DELETE(m_wzName);
	}

void FastIStream::SetSize(unsigned int i)
	{
	if (i > m_cMax)
		{
		void *m_rgNew;

		if (m_rg)
			{
			m_rgNew = realloc((void *)m_rg, sizeof(void *) * (i));
			}
		else
			{
			m_rgNew = malloc(sizeof(void *) * i);
			}

		/*if (m_rgNew == NULL)
			{
			return fFalse;
			}*/
		m_rg = (char *)m_rgNew;
		m_cMax = i;
		}
	}

long __stdcall FastIStream::QueryInterface(const struct _GUID &,void ** )
	{
	return S_OK;
	}

unsigned long __stdcall FastIStream::AddRef(void)
	{
	m_cref++;

	return S_OK;
	}

unsigned long __stdcall FastIStream::Release(void)
	{
	m_cref--;

	if (m_cref == 0)
		{
		delete this;
		}

	return S_OK;
	}

long __stdcall FastIStream::Read(void *pv,unsigned long count,unsigned long *foo)
	{
	memcpy(pv, m_rg + m_cSeek, count);
	m_cSeek += count;
	
	if (foo != NULL)
		{
		*foo = count;
		}

	return S_OK;
	}

long __stdcall FastIStream::Write(const void *pv,unsigned long count,unsigned long *foo)
	{
	if((m_cSeek + count) > m_cMax)
		{
		SetSize(max(m_cSeek*2, m_cSeek + count));
		/*if (!SetSize(max(m_cSeek*2, m_cSeek + count)))
			return -1;*/
		}

	memcpy(m_rg + m_cSeek, pv, count);
	m_cSeek += count;
	
	m_cSize = max(m_cSize, m_cSeek);
	
	if (foo != NULL)
		{
		*foo = count;
		}

	return S_OK;
	}

long __stdcall FastIStream::Seek(union _LARGE_INTEGER li, unsigned long origin, union _ULARGE_INTEGER *puiOut)
	{
	switch (origin)
		{
		case STREAM_SEEK_SET:
			m_cSeek = li.LowPart;
			break;

		case STREAM_SEEK_CUR:
			m_cSeek += li.LowPart;
			break;
		}

	if (puiOut)
		{
		puiOut->QuadPart = m_cSeek;
		}

	return S_OK;
	}

long __stdcall FastIStream::SetSize(union _ULARGE_INTEGER)
	{
	return S_OK;
	}

long __stdcall FastIStream::CopyTo(struct IStream *,union _ULARGE_INTEGER,union _ULARGE_INTEGER *,union _ULARGE_INTEGER *)
	{
	return S_OK;
	}

long __stdcall FastIStream::Commit(unsigned long)
	{
	return S_OK;
	}

long __stdcall FastIStream::Revert(void)
	{
	return S_OK;
	}

long __stdcall FastIStream::LockRegion(union _ULARGE_INTEGER,union _ULARGE_INTEGER,unsigned long)
	{
	return S_OK;
	}

long __stdcall FastIStream::UnlockRegion(union _ULARGE_INTEGER,union _ULARGE_INTEGER,unsigned long)
	{
	return S_OK;
	}

long __stdcall FastIStream::Stat(struct tagSTATSTG *,unsigned long)
	{
	return S_OK;
	}

long __stdcall FastIStream::Clone(struct IStream ** )
	{
	return S_OK;
	}