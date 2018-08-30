
#include "VoltVarCtrl.hpp"

#include <iostream>
#include <fstream>
#include "CLogger.hpp"
#include "Messages.hpp"
#include "CTimings.hpp"
#include "CDeviceManager.hpp"
#include "CGlobalPeerList.hpp"
#include "gm/GroupManagement.hpp"
#include "CGlobalConfiguration.hpp"

#include <sstream>

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <armadillo>


namespace freedm {
	
namespace broker {
		
namespace vvc { 

namespace {
/// This file's logger.
CLocalLogger Logger(__FILE__);
}

///////////////////////////////////////////////////////////////////////////////
/// VVCAgent
/// @description: Constructor for the VVC module
/// @pre: Posix Main should register read handler and invoke this module
/// @post: Object is initialized and ready to run 
/// @param uuid_: This object's uuid
/// @param broker: The Broker
/// @limitations: None
///////////////////////////////////////////////////////////////////////////////
VVCAgent::VVCAgent()
    : ROUND_TIME(boost::posix_time::milliseconds(CTimings::Get("LB_ROUND_TIME")))
    , REQUEST_TIMEOUT(boost::posix_time::milliseconds(CTimings::Get("LB_REQUEST_TIMEOUT")))
{

  Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

  m_RoundTimer = CBroker::Instance().AllocateTimer("vvc");
  m_WaitTimer = CBroker::Instance().AllocateTimer("vvc");

}
VVCAgent::~VVCAgent()
{
}
			
////////////////////////////////////////////////////////////
/// Run
/// @description Main function which initiates the algorithm
/// @pre: Posix Main should invoke this function
/// @post: Triggers the vvc algorithm by calling VVCManage()
/// @limitations None
/////////////////////////////////////////////////////////
int VVCAgent::Run()
{
  std::cout<< " --------------------VVC ---------------------------------" << std::endl; 
  CBroker::Instance().Schedule("vvc",
      boost::bind(&VVCAgent::FirstRound, this, boost::system::error_code()));
  Logger.Info << "VVC is scheduled for the next phase." << std::endl;
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
/// HandleIncomingMessage
/// "Downcasts" incoming messages into a specific message type, and passes the
/// message to an appropriate handler.
/// @pre None
/// @post The message is handled by the target handler or a warning is
///     produced.
/// @param m the incoming message
/// @param peer the node that sent this message (could be this DGI)
///////////////////////////////////////////////////////////////////////////////
void VVCAgent::HandleIncomingMessage(boost::shared_ptr<const ModuleMessage> m, CPeerNode peer)
{
    if(m->has_volt_var_message())
    {
        VoltVarMessage vvm = m->volt_var_message();
        if(vvm.has_voltage_delta_message())
        {
            HandleVoltageDelta(vvm.voltage_delta_message(), peer);
        }
        else if(vvm.has_line_readings_message())
        {
            HandleLineReadings(vvm.line_readings_message(), peer);
        }
	else if(vvm.has_gradient_message())
        {
            HandleGradient(vvm.gradient_message(), peer);
        }
        else
        {
            Logger.Warn << "Dropped unexpected volt var message: \n" << m->DebugString();
        }
    }
    else if(m->has_group_management_message())
    {
        gm::GroupManagementMessage gmm = m->group_management_message();
        if(gmm.has_peer_list_message())
        {
            HandlePeerList(gmm.peer_list_message(), peer);
        }
        else
        {
            Logger.Warn << "Dropped unexpected group management message:\n" << m->DebugString();
        }
    }
    else
    {
        Logger.Warn<<"Dropped message of unexpected type:\n" << m->DebugString();
    }
}

void VVCAgent::HandleVoltageDelta(const VoltageDeltaMessage & m, CPeerNode peer)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    Logger.Notice << "Got VoltageDelta from: " << peer.GetUUID() << std::endl;
    Logger.Notice << "CF "<<m.control_factor()<<" Phase "<<m.phase_measurement()<<std::endl;
}

void VVCAgent::HandleLineReadings(const LineReadingsMessage & m, CPeerNode peer)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    Logger.Notice << "Got Line Readings from "<< peer.GetUUID() << std::endl;
}

void VVCAgent::HandleGradient(const GradientMessage & m, CPeerNode peer)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    Logger.Notice << "Got Gradients from "<< peer.GetUUID() << std::endl;
    Logger.Notice << "size of vector "<< m.gradient_value_size() << std::endl;
    Logger.Notice << "the 1st element = " << m.gradient_value(0) <<std::endl;
    
}


// HandlePeerlist Implementation
void VVCAgent::HandlePeerList(const gm::PeerListMessage & m, CPeerNode peer)
{
	Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
	Logger.Notice << "Updated Peer List Received from: " << peer.GetUUID() << std::endl;
	
	m_peers.clear();
	
	m_peers = gm::GMAgent::ProcessPeerList(m);
	m_leader = peer.GetUUID();
}

// Preparing Messages
ModuleMessage VVCAgent::VoltageDelta(unsigned int cf, float pm, std::string loc)
{
	VoltVarMessage vvm;
	VoltageDeltaMessage *vdm = vvm.mutable_voltage_delta_message();
	vdm -> set_control_factor(cf);
	vdm -> set_phase_measurement(pm);
	vdm -> set_reading_location(loc);
	return PrepareForSending(vvm,"vvc");
}

ModuleMessage VVCAgent::LineReadings(std::vector<float> vals)
{
	VoltVarMessage vvm;
	std::vector<float>::iterator it;
	LineReadingsMessage *lrm = vvm.mutable_line_readings_message();
	for (it = vals.begin(); it != vals.end(); it++)
	{
		lrm -> add_measurement(*it);
	}
	lrm->set_capture_time(boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time()));
	return PrepareForSending(vvm,"vvc");
}

ModuleMessage VVCAgent::Gradient(arma::mat grad)
{
	VoltVarMessage vvm;
	unsigned int idx;
        GradientMessage *grdm = vvm.mutable_gradient_message();
	for (idx = 0; idx < grad.n_rows; idx++)
	{
		grdm -> add_gradient_value(grad(idx));
	}
	grdm->set_gradient_capture_time(boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time()));
	return PrepareForSending(vvm,"vvc");
}


ModuleMessage VVCAgent::PrepareForSending(const VoltVarMessage& message, std::string recipient)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    ModuleMessage mm;
    mm.mutable_volt_var_message()->CopyFrom(message);
    mm.set_recipient_module(recipient);
    return mm;
}
// End of Preparing Messages

///////////////////////////////////////////////////////////////////////////////
/// FirstRound
/// @description The code that is executed as part of the first VVC
///     each round.
/// @pre None
/// @post if the timer wasn't cancelled this function calls the first VVC.
/// @param error The reason this function was called.
///////////////////////////////////////////////////////////////////////////////
void VVCAgent::FirstRound(const boost::system::error_code& err)
{
  Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
  
  if(!err)
  {
    CBroker::Instance().Schedule("vvc", 
	boost::bind(&VVCAgent::VVCManage, this, boost::system::error_code()));
  }
  else if(err == boost::asio::error::operation_aborted)
  {
    Logger.Notice << "VVCManage Aborted" << std::endl;
  }
  else
  {
    Logger.Error << err << std::endl;
    throw boost::system::system_error(err);
  }
			
}


////////////////////////////////////////////////////////////
/// VVCManage
/// @description: Manages the execution of the VVC algorithm 
/// @pre: 
/// @post: 
/// @peers 
/// @limitations
/////////////////////////////////////////////////////////
void VVCAgent::VVCManage(const boost::system::error_code& err)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

    if(!err)
    {
      ScheduleNextRound();
      ReadDevices();
      vvc_main();
     
      
    }
        
    else if(err == boost::asio::error::operation_aborted)
    {
        Logger.Notice << "VVCManage Aborted" << std::endl;
    }
    else
    {
        Logger.Error << err << std::endl;
        throw boost::system::system_error(err);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// ScheduleNextRound
/// @description Computes how much time is remaining and if there isn't enough
///     requests the VVC that will run next round.
/// @pre None
/// @post VVCManage is scheduled for this round OR FirstRound is scheduled
///     for next time.
///////////////////////////////////////////////////////////////////////////////
void VVCAgent::ScheduleNextRound()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

    if(CBroker::Instance().TimeRemaining() > ROUND_TIME + ROUND_TIME)
    {
        CBroker::Instance().Schedule(m_RoundTimer, ROUND_TIME,
            boost::bind(&VVCAgent::VVCManage, this, boost::asio::placeholders::error));
        Logger.Info << "VVCManage scheduled in " << ROUND_TIME << " ms." << std::endl;
    }
    else
    {
        CBroker::Instance().Schedule(m_RoundTimer, boost::posix_time::not_a_date_time,
            boost::bind(&VVCAgent::FirstRound, this, boost::asio::placeholders::error));
        Logger.Info << "VVCManage scheduled for the next phase." << std::endl;
    }
}


///////////////////////////////////////////////////////////////////////////////
/// ReadDevices
/// @description Reads the device state and updates the appropriate member vars.
/// @pre None
/// @post 
///////////////////////////////////////////////////////////////////////////////
void VVCAgent::ReadDevices()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

    float generation = device::CDeviceManager::Instance().GetNetValue("Drer", "generation");
    float storage = device::CDeviceManager::Instance().GetNetValue("Desd", "storage");
    float load = device::CDeviceManager::Instance().GetNetValue("Load", "drain");

    m_Gateway = device::CDeviceManager::Instance().GetNetValue("Sst", "gateway");
    m_NetGeneration = generation + storage - load;
}







void VVCAgent::vvc_main()
{
using namespace arma;
using namespace std;
	
//Prepare para for DPF
sysdata sysinfo = load_system_data();
int Ldl = sysinfo.Dl.n_rows;
int Wdl = sysinfo.Dl.n_cols;
cout << "Dl dimension:"<< Ldl <<"*"<< Wdl << endl;//Matrix Dl in Matlab
mat Dl = sysinfo.Dl;
cx_mat Z = sysinfo.Z;

mat du, step_size; // delta control
double Ploss_aftter_ctrl;
bool flag = true;
double Vmax, Vmin;

//document the original node number in sequence
int j, ja, jb, jc, ia, ib, ic;

j = 1;
ja = 0;
jb = 0;
jc = 0;

int cnt_nodes = 0;
int Lla = 0;
int Llb = 0;
int Llc = 0;
for (int i = 0; i < Ldl; ++i)
{
  if ((int)Dl(i, 0) != 0)
  cnt_nodes = cnt_nodes + 1;
  if ((int)Dl(i, 6) != 0)
    Lla = Lla + 1;
  if ((int)Dl(i, 8) != 0)
    Llb = Llb + 1;
  if ((int)Dl(i, 10) != 0)
    Llc = Llc + 1;
}
cnt_nodes = cnt_nodes + 1;//No of nodes= No of branches +1
mat Node_f = zeros(1,cnt_nodes);
mat Load_a = zeros(1, Lla);
mat Load_b = zeros(1, Llb);
mat Load_c = zeros(1, Llc);

//cout << "Lla: " << Lla << " Llb: " << Llb << " Llc: " << Llc << endl;

for (int i = 0; i < Ldl && j<cnt_nodes && ja<Lla && jb<Llb &&jc<Llc; ++i)
{
  if (	(int)Dl(i, 2) != 0  )
  {
    Node_f(0, j) = Dl(i, 2);
    ++j;
  }
  if ((int)Dl(i, 6) != 0)
  {
    Load_a(ja) = Dl(i, 2);
    ++ja;
  }
  if ((int)Dl(i, 8) != 0)
  {
    Load_b(jb) = Dl(i, 2);
    ++jb;
  }
  if ((int)Dl(i, 10) != 0)
  {
    Load_c(jc) = Dl(i, 2);
    ++jc;
  }
}
Node_f = Node_f.st();
//std::cout << "Node_f: " << endl << Node_f << endl;

y_re Y_return = form_Y_abc(Dl, sysinfo.Z, sysinfo.bkva, sysinfo.bkv);
cout << "Phase A B C NoBranches:" << Y_return.Lnum << endl;
cout << "No. of Nodes:" << Y_return.Nnum << endl;
	
int Lbr = Y_return.brnches.n_rows;
int Wbr = Y_return.brnches.n_cols;

//save three phase branch data separately
cx_mat brn_a = cx_mat(zeros(Y_return.Lnum_a, Y_return.brnches.n_cols), zeros(Y_return.Lnum_a, Y_return.brnches.n_cols));
cx_mat brn_b = cx_mat(zeros(Y_return.Lnum_b, Y_return.brnches.n_cols), zeros(Y_return.Lnum_b, Y_return.brnches.n_cols));
cx_mat brn_c = cx_mat(zeros(Y_return.Lnum_c, Y_return.brnches.n_cols), zeros(Y_return.Lnum_c, Y_return.brnches.n_cols));

ja = 0;
jb = 0;
jc = 0;

for (int i = 0; i < Lbr && ja<Y_return.Lnum_a && jb<Y_return.Lnum_b && jc<Y_return.Lnum_c; ++i)
{
  if (abs(Y_return.brnches(i, 2)) != 0)
  {
    brn_a.row(ja) = Y_return.brnches.row(i);
    ++ja;
  }
  if (abs(Y_return.brnches(i, 3)) != 0)
  {
    brn_b.row(jb) = Y_return.brnches.row(i);
    ++jb;
  }
  if (abs(Y_return.brnches(i, 4)) != 0)
  {
    brn_c.row(jc) = Y_return.brnches.row(i);
    ++jc;
  }
}

//std::cout << "brn_a: " << endl << brn_a << endl;
//std::cout << "brn_b: " << endl << brn_b << endl;
//std::cout << "brn_c: " << endl << brn_c << endl;
/////*********read from RSCAD output***********/////

//Phase A	
// Reading Pload


std::set<device::CDevice::Pointer> ploadaSet;

//retrieve the set of Phase A load devices
ploadaSet = device::CDeviceManager::Instance().GetDevicesOfType("Pload_a");
if( ploadaSet.empty() )
{
  cout << "Error! No Load Phase A!" << endl;
}
try
{
  BOOST_FOREACH(device::CDevice::Pointer ploada, ploadaSet)
  {
    float xx = ploada->GetState("pload");
    cout << " Phase A Load (kW) =  " << xx << "\n";
    cout << "The Device ID is " << ploada->GetID() << endl;
    
    // SST3 (820)
    if (ploada->GetID()=="Pl3_a" && xx == 17)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl3_a")
    {
      Dl(23,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST3 found:   " << Dl(23,6) <<" kW" << endl;
    }
    else
    {}

    // SST4 (822)
    if (ploada->GetID()=="Pl4_a" && xx == 7)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl4_a")
    {
      Dl(24,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST4 found:   " << Dl(24,6) <<" kW" << endl;
    }
    else
    {}

    // SST8 (830)
    if (ploada->GetID()=="Pl8_a" && xx == 4)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl4_a")
    {
      Dl(9,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST8 found:   " << Dl(9,6) <<" kW" << endl;
    }
    else
    {}

    // SST10 (858)
    if (ploada->GetID()=="Pl10_a" && xx == 36)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl10_a")
    {
      Dl(13,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST10 found:   " << Dl(13,6) <<" kW" << endl;
    }
    else
    {}

    // SST11 (890)
    if (ploada->GetID()=="Pl11_a" && xx == 30)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl11_a")
    {
      Dl(31,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST11 found:   " << Dl(31,6) <<" kW" << endl;
    }
    else
    {}

    // SST12 (864)
    if (ploada->GetID()=="Pl12_a" && xx == 34)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl12_a")
    {
      Dl(33,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST12 found:   " << Dl(33,6) <<" kW" << endl;
    }
    else
    {}

    // SST13 (834)
    if (ploada->GetID()=="Pl13_a" && xx == 135)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl13_a")
    {
      Dl(14,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST13 found:   " << Dl(14,6) <<" kW" << endl;
    }
    else
    {}

    // SST14 (860)
    if (ploada->GetID()=="Pl14_a" && xx == 2)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl14_a")
    {
      Dl(15,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST14 found:   " << Dl(15,6) <<" kW" << endl;
    }
    else
    {}

    // SST15 (836)
    if (ploada->GetID()=="Pl15_a" && xx == 144)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl15_a")
    {
      Dl(16,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST15 found:   " << Dl(16,6) <<" kW" << endl;
    }
    else
    {}

    // SST17 (844)
    if (ploada->GetID()=="Pl17_a" && xx == 20)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl17_a")
    {
      Dl(36,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST17 found:   " << Dl(36,6) <<" kW" << endl;
    }
    else
    {}

    // SST19 (848)
    if (ploada->GetID()=="Pl19_a" && xx == 27)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl19_a")
    {
      Dl(38,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST19 found:   " << Dl(38,6) <<" kW" << endl;
    }
    else
    {}

    // SST20 (840)
    if (ploada->GetID()=="Pl20_a" && xx == 150)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploada->GetID()=="Pl20_a")
    {
      Dl(40,6) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST20 found:   " << Dl(40,6) <<" kW" << endl;
    }
    else
    {}
  }
}
catch(std::exception & e)
{
  cout << "Error! Load Phase A did not recognize OUTPUT state!" << endl;
}


std::set<device::CDevice::Pointer> ploadbSet;

//retrieve the set of Phase B load devices
ploadbSet = device::CDeviceManager::Instance().GetDevicesOfType("Pload_b");
if( ploadbSet.empty() )
{
  cout << "Error! No Load Phase B!" << endl;
}
try
{
  BOOST_FOREACH(device::CDevice::Pointer ploadb, ploadbSet)
  {
    float xx = ploadb->GetState("pload");
    cout << " Phase B Load (kW) =  " << xx << "\n";
    cout << "The Device ID is " << ploadb->GetID() << endl;
    
    // SST1 (806)
    if (ploadb->GetID()=="Pl1_b" && xx == 30)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl1_b")
    {
      Dl(1,8) = (double)xx*1.0;
      cout << "Phase B load for SST1 found:   " << Dl(1,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST2 (810)
    if (ploadb->GetID()=="Pl2_b" && xx == 5)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl2_b")
    {
      Dl(20,8) = (double)xx*1.0;
      cout << "Phase B load for SST2 found:   " << Dl(20,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST5 (824)
    if (ploadb->GetID()=="Pl5_b" && xx == 10)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl5_b")
    {
      Dl(7,8) = (double)xx*1.0;
      cout << "Phase B load for SST5 found:   " << Dl(7,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST6 (826)
    if (ploadb->GetID()=="Pl6_b" && xx == 2)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl6_b")
    {
      Dl(26,8) = (double)xx*1.0;
      cout << "Phase B load for SST6 found:   " << Dl(26,8) <<" kW" << endl;
    }
    else
    {}

    
    // SST8 (830)
    if (ploadb->GetID()=="Pl8_b" && xx == 15)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl8_b")
    {
      Dl(9,8) = (double)xx*1.0;
      cout << "Phase B load for SST8 found:   " << Dl(9,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST9 (856)
    if (ploadb->GetID()=="Pl9_b" && xx == 40)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl9_b")
    {
      Dl(28,8) = (double)xx*1.0;
      cout << "Phase B load for SST9 found:   " << Dl(28,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST10 (858)
    if (ploadb->GetID()=="Pl10_b" && xx == 10)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl10_b")
    {
      Dl(13,8) = (double)xx*1.0;
      cout << "Phase B load for SST10 found:   " << Dl(13,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST11 (890)
    if (ploadb->GetID()=="Pl11_b" && xx == 28)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl11_b")
    {
      Dl(31,8) = (double)xx*1.0;
      cout << "Phase B load for SST11 found:   " << Dl(31,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST13 (834)
    if (ploadb->GetID()=="Pl13_b" && xx == 16)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl13_b")
    {
      Dl(14,8) = (double)xx*1.0;
      cout << "Phase B load for SST13 found:   " << Dl(14,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST14 (860)
    if (ploadb->GetID()=="Pl14_b" && xx == 40)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl14_b")
    {
      Dl(15,8) = (double)xx*1.0;
      cout << "Phase B load for SST14 found:   " << Dl(15,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST15 (836)
    if (ploadb->GetID()=="Pl15_b" && xx == 4)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl15_b")
    {
      Dl(16,8) = (double)xx*1.0;
      cout << "Phase B load for SST15 found:   " << Dl(16,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST16 (838)
    if (ploadb->GetID()=="Pl16_b" && xx == 150)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl16_b")
    {
      Dl(18,8) = (double)xx*1.0;
      cout << "Phase B load for SST16 found:   " << Dl(18,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST17 (844)
    if (ploadb->GetID()=="Pl17_b" && xx == 135)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl17_b")
    {
      Dl(36,8) = (double)xx*1.0;
      cout << "Phase B load for SST17 found:   " << Dl(36,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST18 (846)
    if (ploadb->GetID()=="Pl18_b" && xx == 25)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl18_b")
    {
      Dl(37,8) = (double)xx*1.0;
      cout << "Phase B load for SST18 found:   " << Dl(37,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST19 (848)
    if (ploadb->GetID()=="Pl19_b" && xx == 43)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl19_b")
    {
      Dl(38,8) = (double)xx*1.0;
      cout << "Phase B load for SST19 found:   " << Dl(38,8) <<" kW" << endl;
    }
    else
    {}
    
    // SST20 (840)
    if (ploadb->GetID()=="Pl20_b" && xx == 31)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadb->GetID()=="Pl20_b")
    {
      Dl(40,8) = (double)xx*1.0;
      cout << "Phase B load for SST20 found:   " << Dl(40,8) <<" kW" << endl;
    }
    else
    {}
  }
}
catch(std::exception & e)
{
  cout << "Error! Load Phase B did not recognize OUTPUT state!" << endl;
}


std::set<device::CDevice::Pointer> ploadcSet;

//retrieve the set of Phase C load devices
ploadcSet = device::CDeviceManager::Instance().GetDevicesOfType("Pload_c");
if( ploadcSet.empty() )
{
  cout << "Error! No Load Phase C!" << endl;
}
try
{
  BOOST_FOREACH(device::CDevice::Pointer ploadc, ploadcSet)
  {
    float xx = ploadc->GetState("pload");
    cout << " Phase C Load (kW) =  " << xx << "\n";
    cout << "The Device ID is " << ploadc->GetID() << endl;
    
    // SST1 (806)
    if (ploadc->GetID()=="Pl1_c" && xx == 25)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl1_c")
    {
      Dl(1,10) = (double)xx*1.0;
      cout << "Phase C load for SST1 found:   " << Dl(1,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST7 (828)
    if (ploadc->GetID()=="Pl7_c" && xx == 4)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl7_c")
    {
      Dl(8,10) = (double)xx*1.0;
      cout << "Phase C load for SST7 found:   " << Dl(8,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST8 (830)
    if (ploadc->GetID()=="Pl8_c" && xx == 25)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl8_c")
    {
      Dl(9,10) = (double)xx*1.0;
      cout << "Phase C load for SST8 found:   " << Dl(9,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST10 (858)
    if (ploadc->GetID()=="Pl10_c" && xx == 6)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl10_c")
    {
      Dl(13,10) = (double)xx*1.0;
      cout << "Phase C load for SST10 found:   " << Dl(13,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST11 (890)
    if (ploadc->GetID()=="Pl11_c" && xx == 13)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl11_c")
    {
      Dl(31,10) = (double)xx*1.0;
      cout << "Phase C load for SST11 found:   " << Dl(31,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST13 (834)
    if (ploadc->GetID()=="Pl13_c" && xx == 130)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl13_c")
    {
      Dl(14,10) = (double)xx*1.0;
      cout << "Phase C load for SST13 found:   " << Dl(14,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST14 (860)
    if (ploadc->GetID()=="Pl14_c" && xx == 42)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl14_c")
    {
      Dl(15,10) = (double)xx*1.0;
      cout << "Phase C load for SST14 found:   " << Dl(15,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST15 (836)
    if (ploadc->GetID()=="Pl15_c" && xx == 150)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl15_c")
    {
      Dl(16,10) = (double)xx*1.0;
      cout << "Phase C load for SST15 found:   " << Dl(16,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST17 (844)
    if (ploadc->GetID()=="Pl17_c" && xx == 135)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl17_c")
    {
      Dl(36,10) = (double)xx*1.0;
      cout << "Phase C load for SST17 found:   " << Dl(36,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST18 (846)
    if (ploadc->GetID()=="Pl18_c" && xx == 20)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl18_c")
    {
      Dl(37,10) = (double)xx*1.0;
      cout << "Phase C load for SST18 found:   " << Dl(37,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST19 (848)
    if (ploadc->GetID()=="Pl19_c" && xx == 20)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl19_c")
    {
      Dl(38,10) = (double)xx*1.0;
      cout << "Phase C load for SST19 found:   " << Dl(38,10) <<" kW" << endl;
    }
    else
    {}
    
    // SST20 (840)
    if (ploadc->GetID()=="Pl20_c" && xx == 9)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (ploadc->GetID()=="Pl20_c")
    {
      Dl(40,10) = (double)xx*1.0;
      cout << "Phase C load for SST20 found:   " << Dl(40,10) <<" kW" << endl;
    }
    else
    {}
  }
  //cout << Dl.col(10) << endl;
}
catch(std::exception & e)
{
  cout << "Error! Load Phase C did not recognize OUTPUT state!" << endl;
}
// end of Pload reading

// SST reading from RSCAD

std::set<device::CDevice::Pointer> qsstaSet;

//retrieve the set of Phase A load devices
qsstaSet = device::CDeviceManager::Instance().GetDevicesOfType("Sst_a");
if( qsstaSet.empty() )
{
  cout << "Error! No Q load Phase A!" << endl;
}
try
{
  BOOST_FOREACH(device::CDevice::Pointer qssta, qsstaSet)
  {
    float xx = qssta->GetState("gateway");
    cout << " Phase A Load (MVar) =  " << xx << "\n";
    cout << "The Device ID is " << qssta->GetID() << endl;
    // SST3 (820)
    if (qssta->GetID()=="SST3_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST3_a")
    {
      Dl(23,7) = (double)xx*1.0;//ratio depends on the signal from RSCAD
      cout << "Phase A load for SST3 found:   " << Dl(23,7) <<" kVar" << endl;
    }
    else
    {}
    
    // SST4 (822)
    if (qssta->GetID()=="SST4_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST4_a")
    {
      Dl(24,7) = (double)xx*1.0;
      cout << "Phase A load for SST4 found:   " << Dl(24,7) <<" kVar" << endl;
    }
    else
    {}
    
    // SST8 (830)
    if (qssta->GetID()=="SST8_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST8_a")
    {
      Dl(9,7) = (double)xx*1.0;
      cout << "Phase A load for SST8 found:   " << Dl(9,7) <<" kVar" << endl;
    }
    else
    {}
    
    // SST10 (858)
    if (qssta->GetID()=="SST10_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST10_a")
    {
      Dl(13,7) = (double)xx*1.0;
      cout << "Phase A load for SST10 found:   " << Dl(13,7) <<" kVar" << endl;
    }
    else
    {}
    
    // SST11 (890)
    if (qssta->GetID()=="SST11_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST11_a")
    {
      Dl(31,7) = (double)xx*1.0;
      cout << "Phase A load for SST11 found:   " << Dl(31,7) <<" kVar" << endl;
    }
    else
    {}
    
    // SST12 (864)
    if (qssta->GetID()=="SST12_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST12_a")
    {
      Dl(33,7) = (double)xx*1.0;
      cout << "Phase A load for SST12 found:   " << Dl(33,7) <<" kVar" << endl;
    }
    else
    {}
    
    // SST13 (834)
    if (qssta->GetID()=="SST13_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST13_a")
    {
      Dl(14,7) = (double)xx*1.0;
      cout << "Phase A load for SST13 found:   " << Dl(14,7) <<" kVar" << endl;
    }
    else
    {}
	
    // SST14 (860)
    if (qssta->GetID()=="SST14_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST14_a")
    {
      Dl(15,7) = (double)xx*1.0;
      cout << "Phase A load for SST14 found:   " << Dl(15,7) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST15 (836)
    if (qssta->GetID()=="SST15_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST15_a")
    {
      Dl(16,7) = (double)xx*1.0;
      cout << "Phase A load for SST15 found:   " << Dl(16,7) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST17 (844)
    if (qssta->GetID()=="SST17_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST17_a")
    {
      Dl(36,7) = (double)xx*1.0;
      cout << "Phase A load for SST17 found:   " << Dl(36,7) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST19 (848)
    if (qssta->GetID()=="SST19_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST19_a")
    {
      Dl(38,7) = (double)xx*1.0;
      cout << "Phase A load for SST19 found:   " << Dl(38,7) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST20 (840)
    if (qssta->GetID()=="SST20_a" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qssta->GetID()=="SST20_a")
    {
      Dl(40,7) = (double)xx*1.0;
      cout << "Phase A load for SST20 found:   " << Dl(40,7) <<" kVar" << endl;
    }
    else
    {}
  }  
  //cout << Dl.col(6) << endl;
}
catch(std::exception & e)
{
  cout << "Error! Q Load Phase A did not recognize OUTPUT state!" << endl;
}



std::set<device::CDevice::Pointer> qsstbSet;

//retrieve the set of Phase B load devices
qsstbSet = device::CDeviceManager::Instance().GetDevicesOfType("Sst_b");
if( qsstbSet.empty() )
{
  cout << "Error! No Q Load Phase B!" << endl;
}
try
{
  BOOST_FOREACH(device::CDevice::Pointer qsstb, qsstbSet)
  {
    float xx = qsstb->GetState("gateway");
    cout << " Phase B Load (MVar) =  " << xx << "\n";
    cout << "The Device ID is " << qsstb->GetID() << endl;
    
    // SST1 (806)
    if (qsstb->GetID()=="SST1_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST1_b")
    {
      Dl(1,9) = (double)xx*1.0;
      cout << "Phase B load for SST1 found:   " << Dl(1,9) <<" kVar" << endl;
    }
    else
    {}
    
    // SST2 (810)
    if (qsstb->GetID()=="SST2_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST2_b")
    {
      Dl(20,9) = (double)xx*1.0;
      cout << "Phase B load for SST2 found:   " << Dl(20,9) <<" kVar" << endl;
    }
    else
    {}
   
    // SST5 (824)
    if (qsstb->GetID()=="SST5_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST5_b")
    {
      Dl(7,9) = (double)xx*1.0;
      cout << "Phase B load for SST5 found:   " << Dl(7,9) <<" kVar" << endl;
    }
    else
    {}
    
    // SST6 (826)
    if (qsstb->GetID()=="SST6_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST6_b")
    {
      Dl(26,9) = (double)xx*1.0;
      cout << "Phase B load for SST6 found:   " << Dl(26,9) <<" kVar" << endl;
    }
    else
    {}
    
    // SST8 (830)
    if (qsstb->GetID()=="SST8_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST8_b")
    {
      Dl(9,9) = (double)xx*1.0;
      cout << "Phase B load for SST8 found:   " << Dl(9,9) <<" kVar" << endl;
    }
    else
    {}
    
    // SST9 (856)
    if (qsstb->GetID()=="SST9_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST9_b")
    {
      Dl(28,9) = (double)xx*1.0;
      cout << "Phase B load for SST9 found:   " << Dl(28,9) <<" kVar" << endl;
    }
    else
    {}
    
    // SST10 (858)
    if (qsstb->GetID()=="SST10_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST10_b")
    {
      Dl(13,9) = (double)xx*1.0;
      cout << "Phase B load for SST10 found:   " << Dl(13,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST11 (890)
    if (qsstb->GetID()=="SST11_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST11_b")
    {
      Dl(31,9) = (double)xx*1.0;
      cout << "Phase B load for SST11 found:   " << Dl(31,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST13 (834)
    if (qsstb->GetID()=="SST13_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST13_b")
    {
      Dl(14,9) = (double)xx*1.0;
      cout << "Phase B load for SST13 found:   " << Dl(34,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST14 (860)
    if (qsstb->GetID()=="SST14_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST14_b")
    {
      Dl(15,9) = (double)xx*1.0;
      cout << "Phase B load for SST14 found:   " << Dl(15,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST15 (836)
    if (qsstb->GetID()=="SST15_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST15_b")
    {
      Dl(16,9) = (double)xx*1.0;
      cout << "Phase B load for SST15 found:   " << Dl(16,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST16 (838)
    if (qsstb->GetID()=="SST16_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST16_b")
    {
      Dl(18,9) = (double)xx*1.0;
      cout << "Phase B load for SST16 found:   " << Dl(18,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST17 (844)
    if (qsstb->GetID()=="SST17_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST17_b")
    {
      Dl(36,9) = (double)xx*1.0;
      cout << "Phase B load for SST17 found:   " << Dl(36,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST18 (846)
    if (qsstb->GetID()=="SST18_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST18_b")
    {
      Dl(37,9) = (double)xx*1.0;
      cout << "Phase B load for SST18 found:   " << Dl(37,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST19 (848)
    if (qsstb->GetID()=="SST19_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST19_b")
    {
      Dl(38,9) = (double)xx*1.0;
      cout << "Phase B load for SST19 found:   " << Dl(38,9) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST20 (840)
    if (qsstb->GetID()=="SST20_b" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstb->GetID()=="SST20_b")
    {
      Dl(40,9) = (double)xx*1.0;
      cout << "Phase B load for SST20 found:   " << Dl(40,9) <<" kVar" << endl;
    }
    else
    {}
  } 
  //cout << Dl.col(8) << endl;
}
catch(std::exception & e)
{
  cout << "Error! Q Load Phase B did not recognize OUTPUT state!" << endl;
}



std::set<device::CDevice::Pointer> qsstcSet;

//retrieve the set of Phase C load devices
qsstcSet = device::CDeviceManager::Instance().GetDevicesOfType("Sst_c");
if( qsstcSet.empty() )
{
  cout << "Error! No Q Load Phase C!" << endl;
}
try
{
  BOOST_FOREACH(device::CDevice::Pointer qsstc, qsstcSet)
  {
    float xx = qsstc->GetState("gateway");
    cout << " Phase C Load (MVar) =  " << xx << "\n";
    cout << "The Device ID is " << qsstc->GetID() << endl;
    
    // SST1 (806)
    if (qsstc->GetID()=="SST1_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST1_c")
    {
      Dl(1,11) = (double)xx*1.0;
      cout << "Phase C load for SST1 found:   " << Dl(1,11) <<" kVar" << endl;
    }
    else
    {}
    
    // SST7 (828)
    if (qsstc->GetID()=="SST7_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST7_c")
    {
      Dl(8,11) = (double)xx*1.0;
      cout << "Phase C load for SST7 found:   " << Dl(8,11) <<" kVar" << endl;
    }
    else
    {}
    
    // SST8 (830)
    if (qsstc->GetID()=="SST8_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST8_c")
    {
      Dl(9,11) = (double)xx*1.0;
      cout << "Phase C load for SST8 found:   " << Dl(9,11) <<" kVar" << endl;
    }
    else
    {}
    
    // SST10 (858)
    if (qsstc->GetID()=="SST10_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST10_c")
    {
      Dl(13,11) = (double)xx*1.0;
      cout << "Phase C load for SST10 found:   " << Dl(13,11) <<" kVar" << endl;
    }
    else
    {}
    
    // SST11 (890)
    if (qsstc->GetID()=="SST11_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST11_c")
    {
      Dl(31,11) = (double)xx*1.0;
      cout << "Phase C load for SST11 found:   " << Dl(31,11) <<" kVar" << endl;
    }
    else
    {}
    
    // SST13 (834)
    if (qsstc->GetID()=="SST13_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST13_c")
    {
      Dl(14,11) = (double)xx*1.0;
      cout << "Phase C load for SST13 found:   " << Dl(14,11) <<" kVar" << endl;
    }
    else
    {}
    
    // SST14 (860)
    if (qsstc->GetID()=="SST14_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST14_c")
    {
      Dl(15,11) = (double)xx*1.0;
      cout << "Phase C load for SST14 found:   " << Dl(15,11) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST15 (836)
    if (qsstc->GetID()=="SST15_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST15_c")
    {
      Dl(16,11) = (double)xx*1.0;
      cout << "Phase C load for SST15 found:   " << Dl(16,11) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST17 (844)
    if (qsstc->GetID()=="SST17_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST17_c")
    {
      Dl(36,11) = (double)xx*1.0;
      cout << "Phase C load for SST17 found:   " << Dl(36,11) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST18 (846)
    if (qsstc->GetID()=="SST18_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST18_c")
    {
      Dl(37,11) = (double)xx*1.0;
      cout << "Phase C load for SST18 found:   " << Dl(37,11) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST19 (848)
    if (qsstc->GetID()=="SST19_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST19_c")
    {
      Dl(38,11) = (double)xx*1.0;
      cout << "Phase C load for SST19 found:   " << Dl(38,11) <<" kVar" << endl;
    }
    else
    {}
	  
    // SST20 (860)
    if (qsstc->GetID()=="SST20_c" && xx == 0)
    {
      cout << "Signal not updated!" << endl;
    }
    else if (qsstc->GetID()=="SST20_c")
    {
      Dl(40,11) = (double)xx*1.0;
      cout << "Phase C load for SST20 found:   " << Dl(40,11) <<" kVar" << endl;
    }
    else
    {}
  }
  //cout << Dl.col(10) << endl;
}
catch(std::exception & e)
{
  cout << "Error! Q Load Phase C did not recognize OUTPUT state!" << endl;
}

// end of SST reading

// end of reading from RSCAD



VPQ dpf_re = DPF_return34(Dl,Z);
//cout << "Vpolar = \n" << dpf_re.Vpolar << endl;
//cout << "PQb = \n" << dpf_re.Vpolar << endl;
mat Vpolar = dpf_re.Vpolar;
mat PQb = dpf_re.PQb;
mat PQL = dpf_re.PQL;

int Lvp = Vpolar.n_rows;
int Wvp = Vpolar.n_cols;
int Lpq = PQb.n_rows;
int Wpq = PQb.n_cols;
double Pla_t = sum(PQL.col(0));
double Plb_t = sum(PQL.col(2));
double Plc_t = sum(PQL.col(4));

// calculate power loss
mat Pload_total, Ploss_orig_ph;
double Ploss_orig;
Pload_total<< Pla_t << Plb_t << Plc_t << endr;
Ploss_orig_ph << (PQb(0, 0) - Pla_t) << (PQb(0, 2) - Plb_t) << (PQb(0, 4) - Plc_t) << endr;
//cout << "phase loss" << Ploss_orig_ph << endl;
Ploss_orig = accu(Ploss_orig_ph);
//cout << "total load (kW) per phase:" << Pload_total << endl;
//cout << "total loss (kW):" << Ploss_orig << endl;



//document the valid Vs and corrsponding node number for each phase
int Lnum_a = Y_return.Lnum_a;
int Lnum_b = Y_return.Lnum_b;
int Lnum_c = Y_return.Lnum_c;

//cout << "Lnum-a: " << endl << Lnum_a;
//cout << "Lnum-b: " << endl << Lnum_b;
//cout << "Lnum-c: " << endl << Lnum_c;

Vabc V_abc = V_abc_list(Vpolar,Node_f,Lvp,Lnum_a,Lnum_b,Lnum_c);

int Lna = V_abc.Lna;
int Lnb = V_abc.Lnb;
int Lnc = V_abc.Lnc;
mat Node_a = V_abc.Node_a;
mat Node_b = V_abc.Node_b;
mat Node_c = V_abc.Node_c;
mat V_a = V_abc.V_a;
mat V_b = V_abc.V_b;
mat V_c = V_abc.V_c;
mat theta_a = V_abc.theta_a;
mat theta_b = V_abc.theta_b;
mat theta_c = V_abc.theta_c;

//Qset before update
mat Qset_a = dpf_re.Qset_a;
mat Qset_b = dpf_re.Qset_b;
mat Qset_c = dpf_re.Qset_c;
mat Dl_new = Dl;
Dl_new.col(7) = Qset_a;
Dl_new.col(9) = Qset_b;
Dl_new.col(11) = Qset_c;

newbrn Newbrn_return = rename_brn(Node_a, Node_b, Node_c, brn_a, brn_b, brn_c, Y_return.Lnum_a, Y_return.Lnum_b, Y_return.Lnum_c, Lna, Lnb, Lnc);
//cout << "Newbrn_a: " << endl << Newbrn_return.newbrn_a << endl;

//cout << "Phase A Vmin:" << min(V_a) << endl;
//cout << "Phase B Vmin:" << min(V_b) << endl;
//cout << "Phase C Vmin:" << min(V_c) << endl;
mat Vmin_abc, Vmax_abc;
Vmin_abc << min(min(V_a)) << min(min(V_b)) << min(min(V_c))<<endr;
Vmax_abc << max(max(V_a)) << max(max(V_b)) << max(max(V_c))<<endr;
double Vmin_orig = min(min(Vmin_abc));
double Vmax_orig = max(max(Vmax_abc));
//cout << "Vmax (p.u.) = " << Vmax_orig << endl;
//cout << "Vmin (p.u.) = " << Vmin_orig << endl;




// ********************************* phase II: Power Loss Minimization **********************************//
// power loss minimization while at each iteration determine Qlimit
// obj F = sum of line losses

// Gradient Calculation

//double beta0 = 0.1;// min dQsst for SST is 0.1 kVar
//double beta0 = 0.02;
double beta0 = 0.001;
double alpha = 1.1;
int m_max = 300; // max iterations to search for the best step-size
//deltaF/deltaTheta
arma::mat Ftheta_a = form_Ftheta(Y_return.Y_a, V_a, theta_a, Newbrn_return.newbrn_a, Lna, Lnum_a);
arma::mat Ftheta_b = form_Ftheta(Y_return.Y_b, V_b, theta_b, Newbrn_return.newbrn_b, Lnb, Lnum_b);
arma::mat Ftheta_c = form_Ftheta(Y_return.Y_c, V_c, theta_c, Newbrn_return.newbrn_c, Lnc, Lnum_c);

//detltaF/deltaV
arma::mat Fv_a = form_Fv(Y_return.Y_a, V_a, theta_a, Newbrn_return.newbrn_a, Lna, Lnum_a);
arma::mat Fv_b = form_Fv(Y_return.Y_b, V_b, theta_b, Newbrn_return.newbrn_b, Lnb, Lnum_b);
arma::mat Fv_c = form_Fv(Y_return.Y_c, V_c, theta_c, Newbrn_return.newbrn_c, Lnc, Lnum_c);
//detltaF/deltaX
arma::mat Fx_a = join_cols(Ftheta_a, Fv_a);
arma::mat Fx_b = join_cols(Ftheta_b, Fv_b);
arma::mat Fx_c = join_cols(Ftheta_c, Fv_c);
					

//form Jacobian Matrices for each phase
arma::mat J_a, J_b, J_c;
J_a = form_J(Y_return.Y_a, V_a, theta_a, Lna);
J_b = form_J(Y_return.Y_b, V_b, theta_b, Lnb);
J_c = form_J(Y_return.Y_c, V_c, theta_c, Lnc);

// get lambda for each phase
arma::mat lambda_a = -inv(J_a.st())*Fx_a;//need LAPAC etc
arma::mat lambda_b = -inv(J_b.st())*Fx_b;
arma::mat lambda_c = -inv(J_c.st())*Fx_c;

//std::cout << lambda_a << std::endl;
					
//deltaG/delta_Qnj from sst
//deltaP/delta_Qinj
arma::mat Gpq_a = arma::zeros(Lnum_a, Lla);
arma::mat Gpq_b = arma::zeros(Lnum_b, Llb);
arma::mat Gpq_c = arma::zeros(Lnum_c, Llc);
//deltaQ/delta_Qinj
arma::mat Gqq_a = arma::zeros(Lnum_a, Lla);
arma::mat Gqq_b = arma::zeros(Lnum_b, Llb);
arma::mat Gqq_c = arma::zeros(Lnum_c, Llc);
					
					
					//Phase A
					for (ia = 0; ia < Lnum_a; ++ia)
					{
						for ( ja = 0; ja < Lla; ++ja)
						{
							if (Node_a(0, ia + 1) == Load_a(0, ja))
							{
								Gqq_a(ia, ja) = -1;
							}

						}
					}
					
					//Phase B
					for (ib = 0; ib < Lnum_b; ++ib)
					{
						for (jb = 0; jb < Llb; ++jb)
						{
							if (Node_b(0, ib + 1) == Load_b(0, jb))
							{
								Gqq_b(ib, jb) = -1;
							}

						}
					}
					
					//Phase C
					for (ic = 0; ic < Lnum_c; ++ic)
					{
						for (jc = 0; jc < Llc; ++jc)
						{
							if (Node_c(0, ic + 1) == Load_c(0, jc))
							{
								Gqq_c(ic, jc) = -1;
							}

						}
					}
					
//form deltaG/deltaQinj
arma::mat gu_a, gu_b, gu_c;
gu_a = join_cols(Gpq_a, Gqq_a);
gu_b = join_cols(Gpq_b, Gqq_b);
gu_c = join_cols(Gpq_c, Gqq_c);

					

arma::mat g_vq_a = -gu_a.st()*lambda_a;//Gradient in p.u.  aka df/du , u is Qinj
arma::mat g_vq_b = -gu_b.st()*lambda_b;
arma::mat g_vq_c = -gu_c.st()*lambda_c;
std::cout<< g_vq_a << std::endl;
std::cout<< g_vq_b << std::endl;
std::cout<< g_vq_c << std::endl;

arma::mat g_min;
g_min << min(min(abs(g_vq_a))) << min(min(abs(g_vq_b))) << min(min(abs(g_vq_c))) << arma::endr;
arma::mat g_max;
g_max << max(max(abs(g_vq_a))) << max(max(abs(g_vq_b))) << max(max(abs(g_vq_c))) << arma::endr;
double gmax = max(max(g_max));
double gmin = min(min(g_min));
std::cout << "max gradient=" << gmax << "	" << "min grad=" << gmin << "(p.u.)" << std::endl;
double gabs_min = min(min(abs( join_cols( g_vq_a, join_cols( g_vq_b, g_vq_c )) )));
// end of gradient calculation	

//cout << "gabs_min=" << gabs_min << endl;
double cvq_a = beta0/(sysinfo.bkva/3)/gabs_min;
double cvq_b = beta0/(sysinfo.bkva/3)/gabs_min;
double cvq_c = beta0/(sysinfo.bkva/3)/gabs_min;

mat ctrl_o =  Dl;// save the previous control


for ( int m = 0; m < m_max; m++ )
{
  // update Qinj for three phase separately
  double Gupdate;
  //Phase A
  for (ia = 0; ia < Lla; ++ia)
  {
    for (ja = 0; ja < Ldl; ++ja)
    {
      if (Dl(ja, 2) == Load_a(0, ia))
      {
	Gupdate = g_vq_a(ia, 0)*(sysinfo.bkva/3)*cvq_a;
	Dl_new(ja, 7) = ctrl_o(ja, 7) - Gupdate;
      }
     }
  }//end of Phase A
  
  //Phase B
  for (ib = 0; ib < Llb; ++ib)
  {
    for (jb = 0; jb < Ldl; ++jb)
    {
      if (Dl(jb, 2) == Load_b(0, ib))
      {
	Gupdate = g_vq_b(ib, 0)*(sysinfo.bkva / 3)*cvq_b;
	Dl_new(jb, 9) = ctrl_o(jb, 9) - Gupdate;
      }
    }
   }//end of Phase B
   
   //Phase C
  for (ic = 0; ic < Llc; ++ic)
  {
    for (jc = 0; jc < Ldl; ++jc)
    {
      if (Dl(jc, 2) == Load_c(0, ic))
      {
	Gupdate = g_vq_c(ic, 0)*(sysinfo.bkva / 3)*cvq_c;
	Dl_new(jc, 11) = ctrl_o(jc, 11) - Gupdate;
      }
     }
  }// end of Phase C
mat Dl_osize = Dl_new;  
mat du_temp=Dl_osize-ctrl_o;
du = du_temp.cols(6,11);

// cout << "Dl_new = \n" << Dl_new << endl;
// DPF based on Dl_new

VPQ dpf_re = DPF_return34(Dl_osize,Z);
//cout << "Vpolar = \n" << dpf_re.Vpolar << endl;
mat Vpolar = dpf_re.Vpolar;
mat PQb = dpf_re.PQb;
mat PQL = dpf_re.PQL;

int Lvp = Vpolar.n_rows;
int Wvp = Vpolar.n_cols;
int Lpq = PQb.n_rows;
int Wpq = PQb.n_cols;
double Pla_t = sum(PQL.col(0));
double Plb_t = sum(PQL.col(2));
double Plc_t = sum(PQL.col(4));

// calculate power loss
mat Pload_total, Ploss_oph;
double Ploss_osize;
Pload_total<< Pla_t << Plb_t << Plc_t << endr;
Ploss_oph << (PQb(0, 0) - Pla_t) << (PQb(0, 2) - Plb_t) << (PQb(0, 4) - Plc_t) << endr;
Ploss_osize = accu(Ploss_oph);
//cout << "total load (kW) per phase:" << Pload_total << endl;
//cout << "total loss (kW):" << Ploss_osize << endl;

Vabc V_abc = V_abc_list(Vpolar,Node_f,Lvp,Lnum_a,Lnum_b,Lnum_c);


mat V_a = V_abc.V_a;
mat V_b = V_abc.V_b;
mat V_c = V_abc.V_c;

mat Vmin_abc, Vmax_abc;
Vmin_abc << min(min(V_a)) << min(min(V_b)) << min(min(V_c))<<endr;
Vmax_abc << max(max(V_a)) << max(max(V_b)) << max(max(V_c))<<endr;
Vmin = min(min(Vmin_abc));
Vmax = max(max(Vmax_abc));


step_size << cvq_a << cvq_b << cvq_c << endr;
//cout << "\n \n step-size at the " << m+1 << "th iteration = " << step_size(0,0) << endl;

// new step-size
cvq_a=alpha*cvq_a;
cvq_b=alpha*cvq_b;
cvq_c=alpha*cvq_c; 

//update Dl_new based on the new step-size
//Phase A
  for (ia = 0; ia < Lla; ++ia)
  {
    for (ja = 0; ja < Ldl; ++ja)
    {
      if (Dl(ja, 2) == Load_a(0, ia))
      {
	Gupdate = g_vq_a(ia, 0)*(sysinfo.bkva/3)*cvq_a;
	Dl_new(ja, 7) = ctrl_o(ja, 7) - Gupdate;
      }
     }
  }//end of Phase A
  
  //Phase B
  for (ib = 0; ib < Llb; ++ib)
  {
    for (jb = 0; jb < Ldl; ++jb)
    {
      if (Dl(jb, 2) == Load_b(0, ib))
      {
	Gupdate = g_vq_b(ib, 0)*(sysinfo.bkva / 3)*cvq_b;
	Dl_new(jb, 9) = ctrl_o(jb, 9) - Gupdate;
      }
    }
   }//end of Phase B
   
   //Phase C
  for (ic = 0; ic < Llc; ++ic)
  {
    for (jc = 0; jc < Ldl; ++jc)
    {
      if (Dl(jc, 2) == Load_c(0, ic))
      {
	Gupdate = g_vq_c(ic, 0)*(sysinfo.bkva / 3)*cvq_c;
	Dl_new(jc, 11) = ctrl_o(jc, 11) - Gupdate;
      }
     }
  }// end of Phase C
  
mat Dl_nsize = Dl_new;  

dpf_re = DPF_return34(Dl_nsize,Z);// run DPF based on new step-size
Vpolar = dpf_re.Vpolar;
PQb = dpf_re.PQb;
PQL = dpf_re.PQL;


Pla_t = sum(PQL.col(0));
Plb_t = sum(PQL.col(2));
Plc_t = sum(PQL.col(4));

// calculate power loss
mat Ploss_nph;
double Ploss_nsize;
Pload_total<< Pla_t << Plb_t << Plc_t << endr;
Ploss_nph << (PQb(0, 0) - Pla_t) << (PQb(0, 2) - Plb_t) << (PQb(0, 4) - Plc_t) << endr;
Ploss_nsize = accu(Ploss_nph);
//cout << "total loss (kW):" << Ploss_nsize << endl;

if (Ploss_nsize>Ploss_osize)
{
  Dl = Dl_osize;//save the Dl based on old size
  Ploss_aftter_ctrl = Ploss_osize; 
  du;
  cout << " Best step-size obtained! Searching ends at the " << m+1 << "th iteration \n";
  cout << " Expected power loss (kW) = " << Ploss_aftter_ctrl << endl;
  cout << " Expected power loss reduction (kW) = " << Ploss_orig - Ploss_aftter_ctrl << endl;
  step_size;
 
  // send messages to slaves
  if (Ploss_osize < Ploss_orig)// grad message will NOT be sent to slaves if loss is not reduced
  {
  		BOOST_FOREACH(CPeerNode peer, m_peers | boost::adaptors::map_values)
        	{
      	    
	   	 ModuleMessage mm = VoltageDelta(2, 3.0, "NCSU");
            	peer.Send(mm);
			
			
	    	mat S2;
//	    	S2 << Dl(1,7) << Dl(2,7) << Dl(3,7) << Dl(4,7) << Dl(6,7) << Dl(7,7) << Dl(8,7) << Dl(1,9) << Dl(2,9) << Dl(3,9) << Dl(4,9) << Dl(6,9) << Dl(7,9) << Dl(8,9) << Dl(1,11)<< Dl(2,11)<< Dl(3,11)<< Dl(4,11)<< Dl(6,11)<< Dl(7,11)<< Dl(8,11)<< endr;
		S2 << Dl(9,7) << Dl(13,7) << Dl(14,7) << Dl(15,7) << Dl(16,7) << Dl(23,7) << Dl(24,7) << Dl(31,7) << Dl(33,7) << Dl(36,7) << Dl	(38,7) << Dl(40,7) << Dl(1,9) << Dl(7,9) << Dl(9,9) << Dl(13,9) << Dl(14,9) << Dl(15,9) << Dl(16,9) << Dl(18,9) << Dl(20,9) << Dl(26,9) << Dl(28,9) << Dl(31,9) << Dl(36,9) << Dl(37,9) << Dl(38,9) << Dl(40,9) << Dl(1,11) << Dl(8,11) << Dl(9,11) << Dl(13,11) << Dl(14,11) << Dl(15,11) << Dl(16,11) << Dl(31,11) << Dl(36,11) << Dl(37,11) << Dl(38,11) << Dl(40,11) << endr;
	    	S2 = S2.t();
	    	ModuleMessage mg = Gradient(S2);
	    	peer.Send(mg);
	    
		}
  }
  else
  {
  }
	
  break;
}
else
{
 Ploss_aftter_ctrl = Ploss_osize; 
}
  
if ( m == m_max )
{
  cout << " Unable to obtain the best step-size withing " << m+1 << "th iterations \n";
  break;
}

if ( Ploss_aftter_ctrl > Ploss_orig )
{
  cout << " Direction of gradients need to be reversed! \n";
  flag = false;
  Vmin = Vmin_orig;
  Vmax = Vmax_orig;
}
else
{ 
}



}// end of step-size search

if (flag == false)
{
  cvq_a=-beta0/(sysinfo.bkva/3)/gabs_min; 
  cvq_b=-beta0/(sysinfo.bkva/3)/gabs_min;
  cvq_c=-beta0/(sysinfo.bkva/3)/gabs_min;
  flag = true;
  
for ( int m = 0; m < m_max; m++ )
{
  // update Qinj for three phase separately
  double Gupdate;
  //Phase A
  for (ia = 0; ia < Lla; ++ia)
  {
    for (ja = 0; ja < Ldl; ++ja)
    {
      if (Dl(ja, 2) == Load_a(0, ia))
      {
	Gupdate = g_vq_a(ia, 0)*(sysinfo.bkva/3)*cvq_a;
	Dl_new(ja, 7) = ctrl_o(ja, 7) - Gupdate;
      }
     }
  }//end of Phase A
  
  //Phase B
  for (ib = 0; ib < Llb; ++ib)
  {
    for (jb = 0; jb < Ldl; ++jb)
    {
      if (Dl(jb, 2) == Load_b(0, ib))
      {
	Gupdate = g_vq_b(ib, 0)*(sysinfo.bkva / 3)*cvq_b;
	Dl_new(jb, 9) = ctrl_o(jb, 9) - Gupdate;
      }
    }
   }//end of Phase B
   
   //Phase C
  for (ic = 0; ic < Llc; ++ic)
  {
    for (jc = 0; jc < Ldl; ++jc)
    {
      if (Dl(jc, 2) == Load_c(0, ic))
      {
	Gupdate = g_vq_c(ic, 0)*(sysinfo.bkva / 3)*cvq_c;
	Dl_new(jc, 11) = ctrl_o(jc, 11) - Gupdate;
      }
     }
  }// end of Phase C
mat Dl_osize = Dl_new;  
mat du_temp=Dl_osize-ctrl_o;
du = du_temp.cols(6,11);

//cout << "Dl_new = \n" << Dl_new << endl;
// DPF based on Dl_new

VPQ dpf_re = DPF_return34(Dl_osize,Z);
//cout << "Vpolar = \n" << dpf_re.Vpolar << endl;
mat Vpolar = dpf_re.Vpolar;
mat PQb = dpf_re.PQb;
mat PQL = dpf_re.PQL;

int Lvp = Vpolar.n_rows;
int Wvp = Vpolar.n_cols;
int Lpq = PQb.n_rows;
int Wpq = PQb.n_cols;
double Pla_t = sum(PQL.col(0));
double Plb_t = sum(PQL.col(2));
double Plc_t = sum(PQL.col(4));

// calculate power loss
mat Pload_total, Ploss_oph;
double Ploss_osize;
Pload_total<< Pla_t << Plb_t << Plc_t << endr;
Ploss_oph << (PQb(0, 0) - Pla_t) << (PQb(0, 2) - Plb_t) << (PQb(0, 4) - Plc_t) << endr;
Ploss_osize = accu(Ploss_oph);
//cout << "total load (kW) per phase:" << Pload_total << endl;
//cout << "total loss (kW):" << Ploss_osize << endl;

Vabc V_abc = V_abc_list(Vpolar,Node_f,Lvp,Lnum_a,Lnum_b,Lnum_c);


mat V_a = V_abc.V_a;
mat V_b = V_abc.V_b;
mat V_c = V_abc.V_c;

mat Vmin_abc, Vmax_abc;
Vmin_abc << min(min(V_a)) << min(min(V_b)) << min(min(V_c))<<endr;
Vmax_abc << max(max(V_a)) << max(max(V_b)) << max(max(V_c))<<endr;
Vmin = min(min(Vmin_abc));
Vmax = max(max(Vmax_abc));


step_size << cvq_a << cvq_b << cvq_c << endr;
//cout << "\n \n step-size at the " << m+1 << "th iteration = " << step_size(0,0) << endl;

// new step-size
cvq_a=alpha*cvq_a;
cvq_b=alpha*cvq_b;
cvq_c=alpha*cvq_c; 

//update Dl_new based on the new step-size
//Phase A
  for (ia = 0; ia < Lla; ++ia)
  {
    for (ja = 0; ja < Ldl; ++ja)
    {
      if (Dl(ja, 2) == Load_a(0, ia))
      {
	Gupdate = g_vq_a(ia, 0)*(sysinfo.bkva/3)*cvq_a;
	Dl_new(ja, 7) = ctrl_o(ja, 7) - Gupdate;
      }
     }
  }//end of Phase A
  
  //Phase B
  for (ib = 0; ib < Llb; ++ib)
  {
    for (jb = 0; jb < Ldl; ++jb)
    {
      if (Dl(jb, 2) == Load_b(0, ib))
      {
	Gupdate = g_vq_b(ib, 0)*(sysinfo.bkva / 3)*cvq_b;
	Dl_new(jb, 9) = ctrl_o(jb, 9) - Gupdate;
      }
    }
   }//end of Phase B
   
   //Phase C
  for (ic = 0; ic < Llc; ++ic)
  {
    for (jc = 0; jc < Ldl; ++jc)
    {
      if (Dl(jc, 2) == Load_c(0, ic))
      {
	Gupdate = g_vq_c(ic, 0)*(sysinfo.bkva / 3)*cvq_c;
	Dl_new(jc, 11) = ctrl_o(jc, 11) - Gupdate;
      }
     }
  }// end of Phase C
  
mat Dl_nsize = Dl_new;  

dpf_re = DPF_return34(Dl_nsize,Z);// run DPF based on new step-size
Vpolar = dpf_re.Vpolar;
PQb = dpf_re.PQb;
PQL = dpf_re.PQL;


Pla_t = sum(PQL.col(0));
Plb_t = sum(PQL.col(2));
Plc_t = sum(PQL.col(4));

// calculate power loss
mat Ploss_nph;
double Ploss_nsize;
Pload_total<< Pla_t << Plb_t << Plc_t << endr;
Ploss_nph << (PQb(0, 0) - Pla_t) << (PQb(0, 2) - Plb_t) << (PQb(0, 4) - Plc_t) << endr;
//cout << "total Phase Loss (kW): " << Ploss_nph << endl;
Ploss_nsize = accu(Ploss_nph);
//cout << "total loss (kW):" << Ploss_nsize << endl;

if (Ploss_nsize>Ploss_osize)
{
  std::cout << Vmax << std::endl;
  std::cout << Vmin << std::endl;
  Dl = Dl_osize;//save the Dl based on old size
  Ploss_aftter_ctrl = Ploss_osize; 
  du;
  cout << " Best step-size obtained! Searching ends at the " << m+1 << "th iteration \n";
  cout << " Expected power loss (kW) = " << Ploss_aftter_ctrl << endl;
  cout << " Expected power loss reduction (kW) = " << Ploss_orig - Ploss_aftter_ctrl << endl;
  step_size;
  
  // send messages to slaves
  if (Ploss_osize < Ploss_orig)// grad message will NOT be sent to slaves if loss is not reduced
  {
  BOOST_FOREACH(CPeerNode peer, m_peers | boost::adaptors::map_values)
        {
      	    
	    ModuleMessage mm = VoltageDelta(2, 3.0, "Gradients reversed!");
            peer.Send(mm);
	    
	   
	    mat S2;
//	    S2 << Dl(1,7) << Dl(2,7) << Dl(3,7) << Dl(4,7) << Dl(6,7) << Dl(7,7) << Dl(8,7) << Dl(1,9) << Dl(2,9) << Dl(3,9) << Dl(4,9) << Dl(6,9) << Dl(7,9) << Dl(8,9) << Dl(1,11)<< Dl(2,11)<< Dl(3,11)<< Dl(4,11)<< Dl(6,11)<< Dl(7,11)<< Dl(8,11)<< endr;
	    S2 << Dl(9,7) << Dl(13,7) << Dl(14,7) << Dl(15,7) << Dl(16,7) << Dl(23,7) << Dl(24,7) << Dl(31,7) << Dl(33,7) << Dl(36,7) << Dl	(38,7) << Dl(40,7) << Dl(1,9) << Dl(7,9) << Dl(9,9) << Dl(13,9) << Dl(14,9) << Dl(15,9) << Dl(16,9) << Dl(18,9) << Dl(20,9) << Dl(26,9) << Dl(28,9) << Dl(31,9) << Dl(36,9) << Dl(37,9) << Dl(38,9) << Dl(40,9) << Dl(1,11) << Dl(8,11) << Dl(9,11) << Dl(13,11) << Dl(14,11) << Dl(15,11) << Dl(16,11) << Dl(31,11) << Dl(36,11) << Dl(37,11) << Dl(38,11) << Dl(40,11) << endr;
	    	
	    S2 = S2.t();
	    ModuleMessage mg = Gradient(S2);
	    peer.Send(mg);
	}
  }
  else
  {
  }
  break;
}
else
{
 Ploss_aftter_ctrl = Ploss_osize; 
}
  
if ( m == m_max )
{
  cout << " Unable to obtain the best step-size withing " << m+1 << "th iterations \n";
  break;
}

if ( Ploss_aftter_ctrl > Ploss_orig )
{
  cout << " Gradient VVC failed! Both directions cannot reduce power loss \n";
  Vmin = Vmin_orig;
  Vmax = Vmax_orig;
}
else
{
}
//cout << "QKVAr: \n" << S2;
}// end of step-size search
//cout << "QKVAr: \n" << S2.t();  
}// end of if flag



}// end of vvc_main()


}//namespace vvc
}// namespace broker
}// namespace freedm
			
