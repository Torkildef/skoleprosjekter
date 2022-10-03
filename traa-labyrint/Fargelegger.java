import java.util.ArrayList;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

class Fargelegger implements Runnable{
  ArrayList<Rute> veien;
  Color farge;

  Fargelegger(ArrayList<Rute> veien, Color farge){
    this.veien = veien;
    this.farge = farge;
  }

  public void run(){

      try
      {
        for(Rute r : this.veien){
          Thread.sleep(10);
          r.setDesign();
        }
      }
      catch(InterruptedException ex)
      {
          Thread.currentThread().interrupt();
      }

    //int antallUtveier = this.utveier.size();
    //this.antUtveierTekst.setText("ANTALL UTVEIER: " + String.valueOf(antallUtveier));
  }
}
