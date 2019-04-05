/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2013 Budiarto Herman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Original work authors (from lte-enb-rrc.cc):
 * - Nicola Baldo <nbaldo@cttc.es>
 * - Marco Miozzo <mmiozzo@cttc.es>
 * - Manuel Requena <manuel.requena@cttc.es>
 *
 * Converted to handover algorithm interface by:
 * - Budiarto Herman <budiarto.herman@magister.fi>
 *
 * Modified by: Lucas Pacheco <lucassidpacheco@gmail.com>
 */

#include <math.h> // for de distance calculation
#include "hove-handover-algorithm.h"
#include <ns3/log.h>
#include <ns3/uinteger.h>
#include "ns3/core-module.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("HoveHandoverAlgorithm");

NS_OBJECT_ENSURE_REGISTERED(HoveHandoverAlgorithm);

///////////////////////////////////////////
// Handover Management SAP forwarder
///////////////////////////////////////////

HoveHandoverAlgorithm::HoveHandoverAlgorithm()
    : m_a2MeasId(0)
    , m_a4MeasId(0)
    , m_servingCellThreshold(30)
    , m_neighbourCellOffset(1)
    , m_handoverManagementSapUser(0)
{
    NS_LOG_FUNCTION(this);
    m_handoverManagementSapProvider = new MemberLteHandoverManagementSapProvider<HoveHandoverAlgorithm>(this);
}

HoveHandoverAlgorithm::~HoveHandoverAlgorithm()
{
    NS_LOG_FUNCTION(this);
}

TypeId
HoveHandoverAlgorithm::GetTypeId()
{
    static TypeId tid = TypeId("ns3::HoveHandoverAlgorithm")
                            .SetParent<LteHandoverAlgorithm>()
                            .SetGroupName("Lte")
                            .AddConstructor<HoveHandoverAlgorithm>()
                            .AddAttribute("ServingCellThreshold",
                                "If the RSRQ of the serving cell is worse than this "
                                "threshold, neighbour cells are consider for handover. "
                                "Expressed in quantized range of [0..34] as per Section "
                                "9.1.7 of 3GPP TS 36.133.",
                                UintegerValue(30),
                                MakeUintegerAccessor(&HoveHandoverAlgorithm::m_servingCellThreshold),
                                MakeUintegerChecker<uint8_t>(0, 34))
                            .AddAttribute("NumberOfUEs",
                                "Number of UEs in the simulation",
                                UintegerValue(60),
                                MakeUintegerAccessor(&HoveHandoverAlgorithm::m_numberOfUEs),
                                MakeUintegerChecker<uint8_t>())
                            .AddAttribute("NeighbourCellOffset",
                                "Minimum offset between the serving and the best neighbour "
                                "cell to trigger the handover. Expressed in quantized "
                                "range of [0..34] as per Section 9.1.7 of 3GPP TS 36.133.",
                                UintegerValue(0),
                                MakeUintegerAccessor(&HoveHandoverAlgorithm::m_neighbourCellOffset),
                                MakeUintegerChecker<uint8_t>())
                            .AddAttribute("Threshold",
                                "lorem ipsum",
                                DoubleValue(0.2),
                                MakeDoubleAccessor(&HoveHandoverAlgorithm::m_threshold),
                                MakeDoubleChecker<double>())
                            .AddAttribute("TimeToTrigger",
                                "Time during which neighbour cell's RSRP "
                                "must continuously higher than serving cell's RSRP "
                                "in order to trigger a handover",
                                TimeValue(MilliSeconds(256)), // 3GPP time-to-trigger median value as per Section 6.3.5 of 3GPP TS 36.331
                                MakeTimeAccessor(&HoveHandoverAlgorithm::m_timeToTrigger),
                                MakeTimeChecker());
    return tid;
}

void HoveHandoverAlgorithm::SetLteHandoverManagementSapUser(LteHandoverManagementSapUser* s)
{
    NS_LOG_FUNCTION(this << s);
    m_handoverManagementSapUser = s;
}

LteHandoverManagementSapProvider*
HoveHandoverAlgorithm::GetLteHandoverManagementSapProvider()
{
    NS_LOG_FUNCTION(this);
    return m_handoverManagementSapProvider;
}

void HoveHandoverAlgorithm::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    /*MEDIDAS BASEADAS EM EVENTO A4, OU SEJA
     *CÉLULA VIZINHA SE TORNA MELHOR QUE THRESHOLD.
     *
     *SE APLICA BEM A NOSSO ALGORITMO.
     */

    NS_LOG_LOGIC(this << " requesting Event A4 measurements"
                      << " (threshold=0)");

    LteRrcSap::ReportConfigEutra reportConfig;
    reportConfig.eventId = LteRrcSap::ReportConfigEutra::EVENT_A4;
    reportConfig.threshold1.choice = LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ;
    reportConfig.threshold1.range = 0; // THRESHOLD BAIXO FACILITA DETECÇÃO
    reportConfig.triggerQuantity = LteRrcSap::ReportConfigEutra::RSRQ;
    //reportConfig.timeToTrigger = m_timeToTrigger.GetMilliSeconds();
    reportConfig.reportInterval = LteRrcSap::ReportConfigEutra::MS480;
    m_a4MeasId = m_handoverManagementSapUser->AddUeMeasReportConfigForHandover(reportConfig);
    LteHandoverAlgorithm::DoInitialize();
}

void HoveHandoverAlgorithm::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete m_handoverManagementSapProvider;
}

void HoveHandoverAlgorithm::DoReportUeMeas(uint16_t rnti,
    LteRrcSap::MeasResults measResults)
{
    NS_LOG_FUNCTION(this << rnti << (uint16_t)measResults.measId);

    EvaluateHandover(rnti, measResults.rsrqResult, (uint16_t)measResults.measId, (uint16_t)measResults.servingCellId);

    if (measResults.haveMeasResultNeighCells
        && !measResults.measResultListEutra.empty()) {
        for (std::list<LteRrcSap::MeasResultEutra>::iterator it = measResults.measResultListEutra.begin();
             it != measResults.measResultListEutra.end();
             ++it) {
            NS_ASSERT_MSG(it->haveRsrqResult == true,
                "RSRQ measurement is missing from cellId " << it->physCellId);
            UpdateNeighbourMeasurements(rnti, it->physCellId, it->rsrqResult);
        }
    }
    else {
        NS_LOG_WARN(this << " Event A4 received without measurement results from neighbouring cells");
    }

} // end of DoReportUeMeas

void HoveHandoverAlgorithm::EvaluateHandover(uint16_t rnti,
    uint8_t servingCellRsrq, uint16_t measId, uint16_t servingCellId)
{
    NS_LOG_FUNCTION(this << rnti << (uint16_t)servingCellRsrq);

    /*------------FIND MEASURES FOR GIVEN RNTI------------*/
    MeasurementTable_t::iterator it1;
    it1 = m_neighbourCellMeasures.find(rnti);

    if (it1 == m_neighbourCellMeasures.end()) {
        //NS_LOG_WARN("Skipping handover evaluation for RNTI " << rnti << " because neighbour cells information is not found");
    }
    else {
        std::cout << "aaaaaaaa";
        static double tm = 0;
        MeasurementRow_t::iterator it2;

        /*------------ASSOCIATE RNTI WITH CELLID------------*/
        int imsi;
        std::stringstream rntiFileName;
        rntiFileName << "./v2x_temp/" << servingCellId << "/" << rnti;
        std::ifstream rntiFile(rntiFileName.str());

        while (rntiFile >> imsi) {
        }
        /*-----------------DEFINE PARAMETERS-----------------*/
        uint16_t bestNeighbourCellId = servingCellId;
        uint16_t bestNeighbourCellRsrq = 0;
        //        uint8_t bestcell = 0;

        int i = 0;

        int n_c = it1->second.size() + 1; //número de células

        //características das células
        double cell[it1->second.size() + 1][4];
        //        double soma_rsrq;
        double soma[n_c];
        double soma_res = 0;

        double qosAtual = 0;
        double qoeAtual = 0;
        //CURRENT QOE VALUE

        std::stringstream qoeRnti;
        qoeRnti << "v2x_temp/qoeTorre" << servingCellId;
        std::ifstream qoeRntiFile;
        qoeRntiFile.open(qoeRnti.str());

        //current qos value
        std::stringstream qosRnti;
        qosRnti << "v2x_temp/qosTorre" << servingCellId;
        std::ifstream qosRntiFile;
        qosRntiFile.open(qosRnti.str());

        if (qoeRntiFile.is_open()) {
            while (qoeRntiFile >> qoeAtual) {
            }
        }

        if (qosRntiFile.is_open()) {
            while (qosRntiFile >> qosAtual) {
            }
        }
        if (qosAtual < 0)
            qosAtual = 0;

        /*----------------neighbor cell values----------------*/
        for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
            std::stringstream qoeFileName;
            std::stringstream qosFileName;
            std::string qoeResult;
            std::string qosResult;

            qoeFileName << "v2x_temp/qoeTorre" << (uint16_t)it2->first;
            qosFileName << "v2x_temp/qosTorre" << (uint16_t)it2->first;

            std::ifstream qosFile(qosFileName.str());
            std::ifstream qoeFile(qoeFileName.str());

            cell[i][0] = (uint16_t)it2->second->m_rsrq;

            if (qoeFile.fail() || qoeFile.peek() == std::ifstream::traits_type::eof())
                cell[i][1] = 0;
            else
                while (qoeFile >> qoeResult)
                    cell[i][1] = stod(qoeResult);

            if (qosFile.fail() || qosFile.peek() == std::ifstream::traits_type::eof())
                cell[i][2] = 0;
            else
                while (qosFile >> qosResult)
                    cell[i][2] = stod(qosResult);

            cell[i][3] = it2->first;

            ++i;
        }
        /*-----------------current cell values-----------------*/
        std::stringstream qoeFileName;
        std::stringstream qosFileName;
        std::string qoeResult;
        std::string qosResult;

        qoeFileName << "./v2x_temp/qoeTorre" << servingCellId;
        qosFileName << "./v2x_temp/qosTorre" << servingCellId;


        std::ifstream qosFile(qosFileName.str());
        std::ifstream qoeFile(qoeFileName.str());

        cell[i][0] = (uint16_t)servingCellRsrq;
        if (qoeAtual)
            cell[i][1] = qoeAtual;
        else {
            cell[i][1] = 0;
        }
        if (qosAtual)
            cell[i][2] = qosAtual;
        else
            cell[i][2] = 0;
        cell[i][3] = servingCellId;

        // multiplicação pelo autovetor [0.14 0.28 0.57]
        // qos > qoe > sinal
        for (int i = 0; i < n_c; ++i) {
            soma[i] =  cell[i][0] * 0.14;
            soma[i] += cell[i][1] * 0.28;
            soma[i] += cell[i][2] * 0.57;
        }

        for (i = 0; i < n_c; ++i) {
            if (soma[i] > soma_res && cell[i][0] != 0 && cell[i][0] > servingCellRsrq) {
                bestNeighbourCellRsrq = cell[i][0];
                bestNeighbourCellId = cell[i][3];
                soma_res = soma[i];
            }
        }

        // Iteraration of the mean distances for each cell in the next 20 seconds
        // todo: orgamize the distances in decreasing order
        PredictPositions(imsi);
        int dist_soma = 0;
        for (int i = 0; i < 12; ++i) {
            dist_soma = 0;
            for (int j = 0; j < 20; ++j)
                dist_soma += dist[i][j];
            std::cout << "Node " << imsi << " is on average " << dist_soma/20  << " meters away from cell " << i + 1 << "\n";
            soma[i] = soma[i] / (dist_soma/20);
        }

        NS_LOG_INFO("\n\n\n------------------------------------------------------------------------");
        NS_LOG_INFO("Measured at: " << Simulator::Now().GetSeconds() << " Seconds.\nIMSI:" << imsi << "\nRNTI: " << rnti << "\n");
        for (int i = 0; i < n_c; ++i) {
            if (cell[i][3] == servingCellId)
                NS_LOG_INFO("Cell " << cell[i][3] << " -- AHP Score:" << soma[i] << " (serving)");
            else
                NS_LOG_INFO("Cell " << cell[i][3] << " -- AHP Score:" << soma[i]);
            NS_LOG_INFO("         -- RSRQ: " << cell[i][0]);
            NS_LOG_INFO("         -- MOSp: " << cell[i][1]);
            NS_LOG_INFO("         -- PDR: " << cell[i][2] << "\n");
        }
        NS_LOG_INFO("\n\nBest Neighbor Cell ID: " << bestNeighbourCellId);

        /*-----------------------------EXECUÇÃO DO HANDOVER-----------------------------*/

        if (bestNeighbourCellId != servingCellId && bestNeighbourCellRsrq > servingCellRsrq) {
            if (Simulator::Now().GetSeconds() - tm < 0.5) {
                NS_LOG_INFO("Last Handover: " << tm);
                NS_LOG_INFO("Delaying Handover");
                NS_LOG_INFO("------------------------------------------------------------------------\n\n\n\n");
                return;
            }
            tm = Simulator::Now().GetSeconds();
            //Simulator::Schedule(Seconds(rand() % 2), &m_handoverManagementSapUser->TriggerHandover, this, rnti, bestNeighbourCellId);
            m_handoverManagementSapUser->TriggerHandover(rnti, bestNeighbourCellId);
            NS_LOG_INFO("Triggering Handover -- RNTI: " << rnti << " -- cellId:" << bestNeighbourCellId << "\n\n\n");
        }
        NS_LOG_INFO("------------------------------------------------------------------------\n\n\n\n");
    } // end of else of if (it1 == m_neighbourCellMeasures.end ())

} // end of EvaluateMeasurementReport

bool HoveHandoverAlgorithm::IsValidNeighbour(uint16_t cellId)
{
    NS_LOG_FUNCTION(this << cellId);

    /**
   * \todo In the future, this function can be expanded to validate whether the
   *       neighbour cell is a valid target cell, e.g., taking into account the
   *       NRT in ANR and whether it is a CSG cell with closed access.
   */

    return true;
}

// function to read mobility files and store them in an array
void HoveHandoverAlgorithm::GetPositions(int imsi, std::string path) 
{
    // coordenadas
    double x, y, z;
    // variaveis auxiliares para a leitura dos arquivos
    std::string aux1, aux2, aux3, aux4, aux5;
    std::string aux_l1, aux_l2, aux_l3;

    // implementação com ponteiros
    // double dist[100][100];
    // std::unique_ptr<double[][20]> dist(new double[12][20]);

    // objeto do arquivo do sumo
    std::ifstream mobilityFile;

    // abertura para leitura
    mobilityFile.open(path, std::ios::in);
    if (mobilityFile.is_open()) {
        // variavel para armazenar a linha atual
        std::string line;
        // enquanto houver linhas
        while (getline(mobilityFile, line)) {
            // string a ser procurada
            std::stringstream at;
            at << "$node_(" << imsi - 1 << ") setdest";

            // se não houver match, não fazer nada
            if (line.find(at.str()) == std::string::npos) {}
            else {
                // separar as colunas da linha
                std::istringstream ss(line);
                ss >> aux1 >> aux2 >> aux3 >> aux4 >> aux5 >> x >> y >> z;

                // ler arquivo com as localizações das células
                std::ifstream infile("cellsList");

                if (stoi(aux3) >= (int) Simulator::Now().GetSeconds() && stoi(aux3) - Simulator::Now().GetSeconds() < 20){
                    int nc = 0;
                    while(infile >> aux_l1 >> aux_l2 >> aux_l3){
                        dist[nc][stoi(aux3)] = sqrt(pow(stod(aux_l2) - x,2) + pow(stod(aux_l3) - y, 2));
                        //NS_LOG_UNCOND("node is " << sqrt(pow(stod(aux_l2) - x,2) + pow(stod(aux_l3) - y, 2)) << " meters away from cell " << nc);
                        ++nc;
                    }
                } // 
                infile.close();
            } // fim da iteração de matches
        } // fim da iteração de linhas
    } // fim da leitura do arquivo de mobilidade
    mobilityFile.close();

    return;
}

void HoveHandoverAlgorithm::PredictPositions(uint16_t imsi)
{
    if (imsi <= m_numberOfUEs / 3)
        GetPositions(imsi, "mobil/taxisTrace.tcl");
    else if (imsi <= m_numberOfUEs * 2 / 3)
        GetPositions(imsi, "mobil/carroTrace.tcl");
    else
        GetPositions(imsi, "mobil/onibusTrace.tcl");
}

void HoveHandoverAlgorithm::UpdateNeighbourMeasurements(uint16_t rnti,
    uint16_t cellId,
    uint8_t rsrq)
{
    NS_LOG_FUNCTION(this << rnti << cellId << (uint16_t)rsrq);
    MeasurementTable_t::iterator it1;
    it1 = m_neighbourCellMeasures.find(rnti);

    if (it1 == m_neighbourCellMeasures.end()) {
        // insert a new UE entry
        MeasurementRow_t row;
        std::pair<MeasurementTable_t::iterator, bool> ret;
        ret = m_neighbourCellMeasures.insert(std::pair<uint16_t, MeasurementRow_t>(rnti, row));
        NS_ASSERT(ret.second);
        it1 = ret.first;
    }

    NS_ASSERT(it1 != m_neighbourCellMeasures.end());
    Ptr<UeMeasure> neighbourCellMeasures;
    std::map<uint16_t, Ptr<UeMeasure> >::iterator it2;
    it2 = it1->second.find(cellId);

    if (it2 != it1->second.end()) {
        neighbourCellMeasures = it2->second;
        neighbourCellMeasures->m_cellId = cellId;
        neighbourCellMeasures->m_rsrp = 0;
        neighbourCellMeasures->m_rsrq = rsrq;
    }
    else {
        // insert a new cell entry
        neighbourCellMeasures = Create<UeMeasure>();
        neighbourCellMeasures->m_cellId = cellId;
        neighbourCellMeasures->m_rsrp = 0;
        neighbourCellMeasures->m_rsrq = rsrq;
        it1->second[cellId] = neighbourCellMeasures;
    }

} // end of UpdateNeighbourMeasurements

} // end of namespace ns3
