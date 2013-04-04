/*
 * gent_frame.h
 *
 *  Created on: 2012-2-19
 *      Author: wyt
 */

#ifndef GENT_FRAME_H_
#define GENT_FRAME_H_
#include "prefine.h"
#include "gent_event.h"
#include "gent_msg.h"
class GentBasic;

class GentFrame
{
	typedef std::map<int, GentBasic *> MODULE_MAP;
private:
	MODULE_MAP modules_;
	static GentFrame *instance_;
public:
	GENT_MSG_CONNECT msg_;
public:
	GentFrame();
	~GentFrame();
public:
	static GentFrame *Instance();
	static void Unstance();
private:
	int Socket();
	int ServerSocket();
public:
    int Init();
	void Run(int count=1);
	int Register(int key, GentBasic *app);
	void Destory();
	int GetModule(GentBasic *&app, int cmd);

};

#endif /* GENT_FRAME_H_ */
