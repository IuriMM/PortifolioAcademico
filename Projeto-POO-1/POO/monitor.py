from POO.matriculados import Matriculados


class Monitor(Matriculados):

  def __init__(self, nome, contaAtiva, valorbolsa, cargahoraria):
    super().__init__()
    self.setNome(nome)
    self.setContaAtiva(contaAtiva)
    self.__valorBolsa = valorbolsa
    self.__cargaHoraria = cargahoraria

  def getTipoEntidade(self):
    return "Monitor"

  def getValorBolsa(self):
    return self.__valorBolsa

  def setValorBolsa(self, novovalor):
    self.__valorBolsa = novovalor
    return self.__valorBolsa

  def imprime(self):
    super().imprime()
    print("Valor da Bolsa: ", self.__valorBolsa)
    print("Carga Horária: ", self.__cargaHoraria)
