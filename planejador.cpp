#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>

#include "planejador.h"

using namespace std;

/* *************************
   * CLASSE IDPONTO        *
   ************************* */

/// Atribuicao de string
void IDPonto::set(string&& S)
{
  t=move(S);
  if (!valid()) t.clear();
}

/* *************************
   * CLASSE IDROTA         *
   ************************* */

/// Atribuicao de string
void IDRota::set(string&& S)
{
  t=move(S);
  if (!valid()) t.clear();
}

/* *************************
   * CLASSE PONTO          *
   ************************* */

/// Distancia entre 2 pontos (formula de haversine)
double Ponto::distancia(const Ponto& P) const
{
  // Gera excecao se pontos invalidos
  if (!valid() || !P.valid())
    throw invalid_argument("distancia: ponto(s) invalido(s)");

  // Tratar logo pontos identicos
  if (id == P.id) return 0.0;
  // Constantes
  static const double MY_PI = 3.14159265358979323846;
  static const double R_EARTH = 6371.0;
  // Conversao para radianos
  double lat1 = MY_PI*this->latitude/180.0;
  double lat2 = MY_PI*P.latitude/180.0;
  double lon1 = MY_PI*this->longitude/180.0;
  double lon2 = MY_PI*P.longitude/180.0;
  // Seno das diferencas
  double sin_dlat2 = sin((lat2-lat1)/2.0);
  double sin_dlon2 = sin((lon2-lon1)/2.0);
  // Quadrado do seno do angulo entre os pontos
  double a = sin_dlat2*sin_dlat2 + cos(lat1)*cos(lat2)*sin_dlon2*sin_dlon2;
  // Em vez de utilizar a funcao arcosseno, asin(sqrt(sin2_ang)),
  // vou utilizar a funcao arcotangente, menos sensivel a erros numericos.
  // Distancia entre os pontos
  return 2.0*R_EARTH*atan2(sqrt(a),sqrt(1-a));
}


/* *************************
   * CLASSE ROTA           *
   ************************* */

/// Retorna a outra extremidade da rota, a que nao eh o parametro.
/// Gera excecao se o parametro nao for uma das extremidades da rota.
IDPonto Rota::outraExtremidade(const IDPonto& ID) const
{
  if (extremidade[0]==ID) return extremidade[1];
  if (extremidade[1]==ID) return extremidade[0];
  throw invalid_argument("outraExtremidade: invalid IDPonto parameter");
}


/* *************************
   * CLASSE PLANEJADOR     *
   ************************* */

/// Torna o mapa vazio
void Planejador::clear()
{
  pontos.clear();
  rotas.clear();
}

/// Retorna um Ponto do mapa, passando a id como parametro.
/// Se a id for inexistente, gera excecao.
Ponto Planejador::getPonto(const IDPonto& Id) const
{
  auto itr = find(pontos.begin(),pontos.end(),Id);
  if (itr!= pontos.end()){
    return *itr;
  }
  throw invalid_argument("getPonto: invalid IDPonto parameter");
}

/// Retorna um Rota do mapa, passando a id como parametro.
/// Se a id for inexistente, gera excecao.
Rota Planejador::getRota(const IDRota& Id) const
{
  auto itr = find(rotas.begin(),rotas.end(),Id);
  if (itr!= rotas.end()){
    return *itr;
  }
  throw invalid_argument("getRota: invalid IDRota parameter");
}

/// Imprime os pontos do mapa no console
void Planejador::imprimirPontos() const
{
  for (const auto& P : pontos)
  {
    cout << P.id << '\t' << P.nome
         << " (" <<P.latitude << ',' << P.longitude << ")\n";
  }
}

/// Imprime as rotas do mapa no console
void Planejador::imprimirRotas() const
{
  for (const auto& R : rotas)
  {
    cout << R.id << '\t' << R.nome << '\t' << R.comprimento << "km"
         << " [" << R.extremidade[0] << ',' << R.extremidade[1] << "]\n";
  }
}

/// Leh um mapa dos arquivos arq_pontos e arq_rotas.
/// Caso nao consiga ler dos arquivos, deixa o mapa inalterado e gera excecao ios_base::failure.
void Planejador::ler(const std::string& arq_pontos,
                     const std::string& arq_rotas)
{
  list<Ponto> pontostemp;
  list<Rota> rotastemp;
  Ponto p;
  Rota r;
  string prov;

  ifstream stream_in(arq_pontos);
  ifstream stream_in2(arq_rotas);

  ///PONTOS

  try
  {
      // tenta abrir o arquivo
      if(!stream_in.is_open()) throw 1;

      // testa o cabecalho
      getline(stream_in, prov);
      if(stream_in.fail() || prov!= "ID;Nome;Latitude;Longitude") throw 2;

      //leitura ate o arquivo acabar
      do{

        // leh id
        getline(stream_in, prov, ';');
        if(stream_in.fail()) throw 3;
        p.id.set(move(prov));

        // leh nome
        getline(stream_in, prov, ';');
        if(stream_in.fail()) throw 4;
        p.nome = move(prov);

        //leh latitude
        stream_in >> p.latitude;
        if(stream_in.fail()) throw 5;
        stream_in.ignore(1,';');

        //leh longitude
        stream_in >> p.longitude;
        stream_in >> ws;
        if(stream_in.fail()) throw 6;

        //verifica se o ponto eh valido
        if(!p.valid()) throw 7;

        //veh se nao tem um ponto repetido na lista
        auto itr = find(pontostemp.begin(),pontostemp.end(),p.id);
        if (itr != pontostemp.end()) throw 8;

        //armazena o ponto na lista
        pontostemp.push_back(move(p));
        p = Ponto();
      }
      while(!stream_in.eof());
  }
  catch(int i)
  {
      string erro = "Erro " + to_string(i) + " na leitura de pontos " + arq_pontos + '\n';
      throw ios_base::failure(erro);
  }
  stream_in.close();

  ///ROTAS

  try
  {
      //tenta abrir o arquivo
      if(!stream_in2.is_open()) throw 1;

      //testa o cabecalho
      getline(stream_in2, prov);
      if(stream_in2.fail() || prov!= "ID;Nome;Extremidade 1;Extremidade 2;Comprimento") throw 2;

      //leitura ate o arquivo acabar
      do{

        // leh id
        getline(stream_in2, prov, ';');
        if(stream_in2.fail()) throw 3;
        r.id.set(move(prov));

        // leh nome
        getline(stream_in2, prov, ';');
        if(stream_in2.fail()) throw 4;
        r.nome = move(prov);

        //leh extremidade1
        getline(stream_in2, prov, ';');
        if(stream_in2.fail()) throw 5;
        r.extremidade[0].set(move(prov));

        //leh extremidade2
        getline(stream_in2, prov, ';');
        if(stream_in2.fail()) throw 6;
        r.extremidade[1].set(move(prov));

        //leh comprimento
        stream_in2 >> r.comprimento;
        stream_in2 >> ws;
        if(stream_in2.fail()) throw 7;

        //verificar se a rota eh valida
        if(!r.valid()) throw 8;

        //verificar se extremidade1 foi lido
        if(!any_of(pontostemp.begin(),pontostemp.end(),[&](const Ponto& p){return p.id == r.extremidade[0];})) throw 9;

        //verificar se extremidade2 foi lido na lista temp
        if(!any_of(pontostemp.begin(),pontostemp.end(),[&](const Ponto& p){return p.id == r.extremidade[1];})) throw 10;

        //veh se nao tem uma rota repetida na lista
        auto itr = find(rotastemp.begin(),rotastemp.end(),r.id);
        if (itr != rotastemp.end()) throw 11;

        //armazena a rota na lista
        rotastemp.push_back(move(r));
        r = Rota();
      }
      while(!stream_in2.eof());
  }
  catch(int i)
  {
       string erro = "Erro " + to_string(i) + " na leitura de rotas " + arq_rotas + '\n';
      throw ios_base::failure(erro);
  }
  stream_in2.close();

  pontos = move(pontostemp);
  rotas = move(rotastemp);
}

/// *******************************************************************************
/// Calcula o caminho entre a origem e o destino do planejador usando o algoritmo A*
/// *******************************************************************************

/// Noh: os elementos dos conjuntos de busca do algoritmo A*


/// Calcula o caminho mais curto no mapa entre origem e destino, usando o algoritmo A*
/// Retorna o comprimento do caminho encontrado (<0 se nao existe caminho).
/// O parametro C retorna o caminho encontrado (vazio se nao existe caminho).
/// O parametro NA retorna o numero de nos (>=0) em Aberto ao termino do algoritmo A*,
/// mesmo quando nao existe caminho.
/// O parametro NF retorna o numero de nos (>=0) em Fechado ao termino do algoritmo A*,
/// mesmo quando nao existe caminho.
/// Em caso de parametros de entrada invalidos ou de erro no algoritmo, gera excecao.
double Planejador::calculaCaminho(const IDPonto& id_origem,
                                  const IDPonto& id_destino,
                                  Caminho& C, int& NA, int& NF)
{
  // Zera o caminho resultado
  C.clear();
  double compr;
  try
  {
    // Mapa vazio
    if (empty()) throw 1;

    Ponto pt_orig, pt_dest;
    // Calcula os pontos que correspondem a id_origem e id_destino.
    // Se algum nao existir, throw 2
    try
    {
      pt_orig = getPonto(id_origem);
      pt_dest = getPonto(id_destino);
    }
    catch(...)
    {
      throw 2;
    }

    /* *****************************  /
    /  IMPLEMENTACAO DO ALGORITMO A*  /
    /  ***************************** */

    //inicializa os conjuntos de nohs
    list<noh> Aberto;
    list<noh> Fechado;

    //noh atual
    noh atual;
    atual.id_pt = id_origem;
    atual.g = 0.0;
    atual.h = pt_orig.distancia(pt_dest);

    Aberto.push_back(atual);

    //laco principal
    do
    {
       //le e exclui o 1o noh de aberto
       atual = Aberto.front();
       Aberto.pop_front();
       //inclui atual em fechado
       Fechado.push_back(atual);
       //expande se nao eh solucao
       if(atual.id_pt != id_destino){
        // gera sucessorres de atual
        list<Rota>::iterator itr_suc = rotas.begin();
        do
        {
            //busca proxima rota conectada a atual
            itr_suc = find_if(itr_suc,rotas.end(),[&atual](const Rota& r){return (atual.id_pt == r.extremidade[0] || atual.id_pt == r.extremidade[1]);});
            //verifica se existe
            if (itr_suc != rotas.end()){
                Rota rota_suc = *itr_suc;
                //atributos do noh_suc
                IDPonto prox_id = rota_suc.outraExtremidade(atual.id_pt);
                Ponto pt_suc = getPonto(prox_id);
                //gerando noh sucessor
                noh noh_suc;
                noh_suc.id_pt = prox_id;
                noh_suc.id_rt = rota_suc.id;
                noh_suc.g = atual.g + rota_suc.comprimento;
                noh_suc.h = pt_suc.distancia(pt_dest);
                //assume que eh inedito
                bool eh_inedito = true;
                //procurando noh suc em fechado
                auto old = find(Fechado.begin(),Fechado.end(),noh_suc.id_pt);
                if(old != Fechado.end()){
                    eh_inedito = false;
                }
                //se nao achou procura em aberto
                else{
                    old = find(Aberto.begin(),Aberto.end(),noh_suc.id_pt);
                    if(old != Aberto.end()){
                        noh prov = *old;
                        if(noh_suc.f() < prov.f()){
                            Aberto.erase(old);
                        }
                        else{
                            eh_inedito = false;
                        }
                    }
                }
                if(eh_inedito){
                    auto big = upper_bound(Aberto.begin(),Aberto.end(),noh_suc);
                    Aberto.insert(big,noh_suc);
                }
                ++itr_suc;
            }
        }
        while(itr_suc!=rotas.end());
       }
    }
    while(!(Aberto.empty()) && (atual.id_pt != id_destino));
    // Calcula nº de nohs da busca
    NA = Aberto.size();
    NF = Fechado.size();

    if(atual.id_pt != id_destino){
        return -1.0;
    }
    else{
        compr = atual.g;

        while(atual.id_rt.valid()){
            C.push_front(Trecho(atual.id_rt,atual.id_pt));
            Rota rota_ant = getRota(atual.id_rt);
            IDPonto id_pt_ant = rota_ant.outraExtremidade(atual.id_pt);
            auto itr = find(Fechado.begin(),Fechado.end(),id_pt_ant);
            atual = *itr;
        }
        C.push_front(Trecho(IDRota(), atual.id_pt));
    }
    return compr;
  }
  catch(int i)
  {
    string msg_err = "Erro " + to_string(i) + " no calculo do caminho\n";
    throw invalid_argument(msg_err);
  }
}
