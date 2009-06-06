//
//
//

//
//
//
void Analyzer::Process()
{
 //
 // Calcula las propiedades temporales
 //
 for (std::vector<TemporalProperty *>::iterator it=tproplist.begin();it!=tproplist.end();++it)
 {
  std::ostream * outp;
  TemporalProperty & prop = *(*it);
  Module & propmod = dynamic_cast<Module &>(prop); // no es necesario CastModule aqui
  if (propmod.Defined("output") && (propmod["output"] != ""))
  {
   outp = new std::ofstream(propmod["output"].c_str());
   if (Verbose()) std::cerr << "-> Writing output of " << propmod.Name() << " to " << propmod["output"] << '\n';
  }
  else outp = &(std::cout);
  Value<Matrix> & propval = CastModule< Value<Matrix> >(propmod);
  IContainable & icont = CastModule<IContainable>(propmod);
  prop.Evaluate(configs, p_array);
  const Matrix & v = propval.CurrentValue();
  (*outp) << v;
  if (propmod.Defined("output") && (propmod["output"] != "")) delete outp;
 }
}

