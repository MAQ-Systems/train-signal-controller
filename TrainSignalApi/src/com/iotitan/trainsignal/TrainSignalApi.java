/**
 * File: TrainSignalApi.java
 * Author: Matt Jones
 * Date: 6/1/2014
 * Desc: REST API for communicating with server program that controls the train signal.
 */

package com.iotitan.trainsignal;

import java.io.IOException;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


public class TrainSignalApi extends HttpServlet {
	
	private static final long serialVersionUID = 06012014L;
    
	@Override
    public void init(ServletConfig config) {
		
    }

	@Override
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		
	}
}
