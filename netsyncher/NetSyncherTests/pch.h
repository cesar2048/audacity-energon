// pch.h: este es un archivo de encabezado precompilado.
// Los archivos que se muestran a continuación se compilan solo una vez, lo que mejora el rendimiento de la compilación en futuras compilaciones.
// Esto también afecta al rendimiento de IntelliSense, incluida la integridad del código y muchas funciones de exploración del código.
// Sin embargo, los archivos que se muestran aquí se vuelven TODOS a compilar si alguno de ellos se actualiza entre compilaciones.
// No agregue aquí los archivos que se vayan a actualizar con frecuencia, ya que esto invalida la ventaja de rendimiento.

#ifndef PCH_H
#define PCH_H
#include <string>
#include "../NetSyncher/HttpProtocol.h"

// Assert that two buffers have the same content
void compareBuffers(uint8_t* expected, uint32_t expectedLen, uint8_t* actual, uint32_t actualLen);

namespace NetSyncherTests {
	// Implements the IOStream interface from a string
	class StrInputStream : public IOStream {
	public:

		char* inputBuffer;
		int inCursor;
		int numberOfReads;

		MemBuffer outBuffer;
		StrInputStream(const char *str);
		~StrInputStream();

		virtual uint32_t peek(uint8_t * buffer, uint32_t len) override;
		virtual uint32_t read(uint8_t * buffer, uint32_t len) override;
		virtual uint32_t write(uint8_t* buffer, uint32_t len) override;
		virtual void close() override;
	};

	// Helper class that reads a network transmission previously recorded (reads buffer by buffer as received)
	class TransmissionReader : public IOStream {
		uint32_t lastFrameLength;
		uint32_t totalRead;
		FILE* f;

		uint32_t readFromFile(uint8_t * buffer, uint32_t len, bool peek);
	public:
		TransmissionReader(char *fileName);
		virtual uint32_t peek(uint8_t * buffer, uint32_t len) override;
		virtual uint32_t read(uint8_t * buffer, uint32_t len) override;
		virtual uint32_t write(uint8_t * buffer, uint32_t len) override;
		virtual void close() override;

		int position();
	};

}

#endif //PCH_H
