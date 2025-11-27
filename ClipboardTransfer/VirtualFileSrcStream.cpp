#include "VirtualFileSrcStream.h"
#include "FileBufferManager.h"
#include <Windows.h>
#include <ShlObj.h>
#include <QtDebug>

namespace clipboard {


	HRESULT STDMETHODCALLTYPE FileStream::QueryInterface(REFIID riid, void **ppvObject)
	{
		if (ppvObject == NULL)
			return E_INVALIDARG;

		*ppvObject = NULL; 

		if (IsEqualIID(IID_IUnknown, riid) ||
			IsEqualIID(IID_ISequentialStream, riid) ||
			IsEqualIID(IID_IStream, riid))
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}

		/*if (IsEqualIID(IID_IOperationsProgressDialog, riid)) {
			return E_NOINTERFACE;
		}*/

		return E_NOINTERFACE;
	}

	HRESULT STDMETHODCALLTYPE FileStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
	{		
		ULONG bytes_to_read = min((ULONG)(file_size_.QuadPart - current_position_.QuadPart), cb);

		if (!pv) {
			return STG_E_INVALIDPOINTER;
		}

		// 从FileBufferManager读取数据
		if (FileBufferManager::instance()) {
			qint64 bytesRead = FileBufferManager::instance()->readData((char*)pv, bytes_to_read);
			current_position_.QuadPart += bytesRead;

			if (pcbRead) {
				*pcbRead = static_cast<ULONG>(bytesRead);
			}

			// 如果没有读取到数据，检查传输是否已完成
			if (bytesRead == 0) {
				// 检查传输是否已完成
				if (FileBufferManager::instance()->isTransferComplete()) {
					// 传输已完成且已到达文件末尾
					return S_FALSE;
				} else {
					// 传输尚未完成，稍后再试
					::Sleep(10); // 短暂等待，避免CPU占用过高
					return S_OK;
				}
			}

			// 如果已读取到数据，检查是否到达文件末尾
			if (current_position_.QuadPart >= file_size_.QuadPart) {
				qDebug() << "read current_position_ >= file_size_";
				return S_FALSE;
			}
			else {
				return S_OK;
			}
		}

		// 如果没有buffer manager，使用原来的假数据代码
		const int BUF_SIZE = 1024 * 1024;
		char *fake_data = new char[BUF_SIZE];
		memset(fake_data, 'F', BUF_SIZE);

		ULONG bytes_now = bytes_to_read;
		while (bytes_now > 0) {
			ULONG to_read_once = min(BUF_SIZE, bytes_now);
			memcpy(pv, fake_data, to_read_once);
			(char* &)pv += to_read_once;
			bytes_now -= to_read_once;
		}
		delete fake_data;
		current_position_.QuadPart += bytes_to_read;

		if (pcbRead) {
			*pcbRead = bytes_to_read;
		}

		/*
		* Always returns S_OK even if the end of the stream is reached before the
		* buffer is filled
		*/
		::Sleep(100);
		return S_FALSE;
	}

	HRESULT STDMETHODCALLTYPE FileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
	{
		ULARGE_INTEGER new_pos = { 0 };

		switch (dwOrigin)
		{
		case STREAM_SEEK_SET:
			break;
		case STREAM_SEEK_CUR:
			new_pos = current_position_;
			break;
		case STREAM_SEEK_END:
			new_pos = file_size_;
			break;
		default:
			return STG_E_INVALIDFUNCTION;
		}

		new_pos.QuadPart += dlibMove.QuadPart;
		if (new_pos.QuadPart < 0 || new_pos.QuadPart > file_size_.QuadPart) {
			return STG_E_INVALIDFUNCTION;
		}

		if (plibNewPosition){
			*plibNewPosition = new_pos;
		}

		current_position_ = new_pos;
		return S_OK;
	}

	HRESULT WINAPI FileStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
	{
		memset(pstatstg, 0, sizeof(STATSTG));

		pstatstg->pwcsName = NULL;
		pstatstg->type = STGTY_STREAM;
		pstatstg->cbSize = file_size_;
		return S_OK;
	}

    //////////////////////////////////////////////////////
    VirtualFileSrcStream::VirtualFileSrcStream()
    {
        qDebug() << "============ create VirtualFileSrcStream";
    }

    VirtualFileSrcStream::~VirtualFileSrcStream()
	{
        FileBufferManager::instance()->clearVFS();
		if (file_stream_) {
			file_stream_->Release();
			file_stream_ = nullptr;
        }
        qDebug() << "************ destroy VirtualFileSrcStream";
    }

    void VirtualFileSrcStream::setFileName(const QString &fileName)
    {
        m_fileName = fileName;
    }

    void VirtualFileSrcStream::onInit()
    {
        clip_format_filedesc_ = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR));
        clip_format_filecontent_ = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILECONTENTS));
        clip_format_remote_file = static_cast<CLIPFORMAT>(RegisterClipboardFormat(L"remoteFile"));

        qDebug() << "clip_format_filedesc_: " << clip_format_filedesc_
                 << "\n clip_format_filecontent_: " << clip_format_filecontent_
                 << "\n clip_format_remote_file: " << clip_format_remote_file;

        m_bInit = true;
    }

    void VirtualFileSrcStream::resetFileStream()
    {
        if (file_stream_) {
            LARGE_INTEGER moveToBegin = {0};
            file_stream_->Seek(moveToBegin, STREAM_SEEK_SET, nullptr);
        }
    }

	bool VirtualFileSrcStream::set_to_clipboard()
	{
		if (clip_format_filedesc_ == 0) {
            onInit();
		}
		return SUCCEEDED(::OleSetClipboard(this));
	}

	STDMETHODIMP VirtualFileSrcStream::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
	{
		ZeroMemory(pmedium, sizeof(*pmedium));

		HRESULT hr = DATA_E_FORMATETC;
		if (pformatetcIn->cfFormat == clip_format_filedesc_)
		{
			if (pformatetcIn->tymed & TYMED_HGLOBAL)
			{
				uint32_t file_count = 1;
				UINT cb = sizeof(FILEGROUPDESCRIPTOR) +	(file_count - 1) * sizeof(FILEDESCRIPTOR);
				HGLOBAL h = GlobalAlloc(GHND | GMEM_SHARE, cb);
				if (!h) {
					hr = E_OUTOFMEMORY;
				}else{
					FILEGROUPDESCRIPTOR* pGroupDescriptor =	(FILEGROUPDESCRIPTOR*)::GlobalLock(h);
					if (pGroupDescriptor)
					{
						pGroupDescriptor->cItems = file_count;
						FILEDESCRIPTOR* pFileDescriptorArray = (FILEDESCRIPTOR*)((LPBYTE)pGroupDescriptor + sizeof(UINT));
						
						for (uint32_t index = 0; index < file_count; ++index) {
							// 从FileBufferManager获取文件名
							wcsncpy_s(pFileDescriptorArray[index].cFileName, _countof(pFileDescriptorArray[index].cFileName), 
                                     m_fileName.toStdWString().c_str(), _TRUNCATE);
							//pFileDescriptorArray[index].dwFlags = FD_UNICODE| FD_FILESIZE | FD_ATTRIBUTES| FD_PROGRESSUI| FD_CREATETIME| FD_SIZEPOINT;
							pFileDescriptorArray[index].dwFlags = FD_FILESIZE | FD_ATTRIBUTES | FD_CREATETIME | FD_WRITESTIME | FD_PROGRESSUI;
							// 从FileBufferManager获取文件大小
							qint64 fileSize = FileBufferManager::instance() ? FileBufferManager::instance()->getFileSize() : 24*1024*1024; // 默认24MB作为后备值
							pFileDescriptorArray[index].nFileSizeLow = fileSize & 0xFFFFFFFF;
							pFileDescriptorArray[index].nFileSizeHigh = (fileSize >> 32) & 0xFFFFFFFF;
							pFileDescriptorArray[index].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

							SYSTEMTIME lt;
							GetLocalTime(&lt);
							FILETIME ft;
							SystemTimeToFileTime(&lt, &ft);
							pFileDescriptorArray[index].ftLastAccessTime = ft;
							pFileDescriptorArray[index].ftCreationTime = ft;
							pFileDescriptorArray[index].ftLastWriteTime = ft;

                            /*pFileDescriptorArray[index].dwFlags = FD_ATTRIBUTES | FD_CREATETIME | FD_WRITESTIME | FD_PROGRESSUI;
                            pFileDescriptorArray[index].dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;*/

						}

						::GlobalUnlock(h);

						pmedium->hGlobal = h;
						pmedium->tymed = TYMED_HGLOBAL;
						hr = S_OK;
					}
					
				}
			}
		} else if (pformatetcIn->cfFormat == clip_format_filecontent_)
		{
			if ((pformatetcIn->tymed & TYMED_ISTREAM))
			{
				// 根据 lindex 确定是哪个文件
				int fileIndex = pformatetcIn->lindex;
				// 从FileBufferManager获取文件大小
				qint64 fileSize = FileBufferManager::instance() ? FileBufferManager::instance()->getFileSize() : 0;
				if (file_stream_ == nullptr) {
					file_stream_ = new FileStream(fileSize);
				}
				else {
					LARGE_INTEGER mov;
					mov.QuadPart = 0;
					file_stream_->Seek(mov, STREAM_SEEK_SET, nullptr);
				}
				pmedium->pstm = (IStream*)file_stream_;
				pmedium->pstm->AddRef();
				pmedium->tymed = TYMED_ISTREAM;
				hr = S_OK;
			}
		}
		else if (SUCCEEDED(_EnsureShellDataObject()))
		{
			hr = _pdtobjShell->GetData(pformatetcIn, pmedium);
		}
		
		return hr;
	}


	STDMETHODIMP VirtualFileSrcStream::QueryGetData(FORMATETC *pformatetc)
	{
		HRESULT hr = S_FALSE;
		if (pformatetc->cfFormat == clip_format_filedesc_ ||
            pformatetc->cfFormat == clip_format_filecontent_||
            pformatetc->cfFormat == clip_format_remote_file)
		{
			hr = S_OK;
		}
		else if (SUCCEEDED(_EnsureShellDataObject()))
		{
			hr = _pdtobjShell->QueryGetData(pformatetc);
		}
		
		return hr;
	}

	STDMETHODIMP VirtualFileSrcStream::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
	{
		*ppenumFormatEtc = NULL;
		HRESULT hr = E_NOTIMPL;
		if (dwDirection == DATADIR_GET)
		{
			FORMATETC rgfmtetc[] =
			{
				// the order here defines the accuarcy of rendering
                { clip_format_filedesc_, NULL, DVASPECT_CONTENT,  -1, TYMED_HGLOBAL },
                { clip_format_filecontent_, NULL, DVASPECT_CONTENT,  -1, TYMED_ISTREAM },

			};
			hr = SHCreateStdEnumFmtEtc(ARRAYSIZE(rgfmtetc), rgfmtetc, ppenumFormatEtc);
		}
		return hr;
	}

	HRESULT STDMETHODCALLTYPE VirtualFileSrcStream::SetAsyncMode(
		/* [in] */ BOOL fDoOpAsync)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE VirtualFileSrcStream::GetAsyncMode(
		/* [out] */ __RPC__out BOOL *pfIsOpAsync)
	{
		*pfIsOpAsync = true;// VARIANT_TRUE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE VirtualFileSrcStream::StartOperation(
		/* [optional][unique][in] */ __RPC__in_opt IBindCtx *pbcReserved)
	{
		in_async_op_ = true;
		IOperationsProgressDialog *pDlg = nullptr;
		::CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IOperationsProgressDialog, (LPVOID*)&pDlg);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE VirtualFileSrcStream::InOperation(
		/* [out] */ __RPC__out BOOL *pfInAsyncOp)
	{
		*pfInAsyncOp = in_async_op_;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE VirtualFileSrcStream::EndOperation(
		/* [in] */ HRESULT hResult,
		/* [unique][in] */ __RPC__in_opt IBindCtx *pbcReserved,
		/* [in] */ DWORD dwEffects)
	{
		if (hResult == HRESULT_FROM_WIN32(ERROR_CANCELLED) ||  hResult == E_ABORT) {
			 // Handle cancellation if needed
			qWarning() << "operaton cancelled or aborted... " << hResult;
			if (m_operCancelledCallback) {
                // 取消粘贴
                m_operCancelledCallback();
			}
		}
        if (SUCCEEDED(hResult)) {
            if (m_operCompletedCallback) {
                m_operCompletedCallback(hResult == S_OK);
            }
        } else {
            qCritical() << "operation failed: " << hResult;
            if (m_abortTransferCallback) {
                // 终止传输
                m_abortTransferCallback();
            }
        }

		if (hResult == S_FALSE) {
			qWarning() << "operaton S_FALSE!!!";
		}
		else if (hResult == S_OK) {
			qInfo() << "operaton S_OK";
        } else {
            qWarning() << "operaton other error: " << hResult;
        }

		in_async_op_ = false;
		return S_OK;
	}


//	STDAPI VirtualFileSrcStream_CreateInstance(REFIID riid, void **ppv)
//	{
//		*ppv = NULL;
//		VirtualFileSrcStream *p = new VirtualFileSrcStream();
//        p->onInit();
//		HRESULT hr = p ? S_OK : E_OUTOFMEMORY;
//		if (SUCCEEDED(hr))
//		{
//			hr = p->QueryInterface(riid, ppv);
//			p->Release();
//		}
//		return hr;
//	}
};
