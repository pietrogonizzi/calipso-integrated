package it.unipr.iot.calipso.tools.stream;

import java.io.IOException;

public interface StreamWriterListener {
	
	public void onLineWritten(String line);
	public void onWriteError(IOException e);

}
