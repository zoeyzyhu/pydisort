.. testcode:: quickstart

   import torch
   from pydisort import Disort, DisortOptions

   torch.set_default_dtype(torch.float64)
   op = DisortOptions().flags("onlyfl,lamber")
   op.ds().nlyr = 4
   op.ds().nstr = 4
   op.ds().nmom = 4
   op.ds().nphase = 4

   ds = Disort(op)
   tau = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
   flux = ds.forward(tau, fbeam=torch.tensor([3.14159]))
   assert flux.shape == (1, 1, 5, 2)
   print(flux)

.. testoutput:: quickstart

   tensor([[[[0.0000, 3.1416],
             [0.0000, 2.8426],
             [0.0000, 2.3273],
             [0.0000, 1.7241],
             [0.0000, 1.1557]]]])
