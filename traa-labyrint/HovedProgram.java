import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;
import java.util.ArrayList;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.filechooser.FileNameExtensionFilter;

class HovedProgram {
    public static void main(String[] args) {
      
      //Ber bruker åpne velge fil fra filvelgeren
      JFileChooser velger = new JFileChooser(".");
      FileNameExtensionFilter filter = new FileNameExtensionFilter("IN files", "in");
      velger.setFileFilter(filter);
      
      int resultat = velger.showOpenDialog(null);
      if(resultat != JFileChooser.APPROVE_OPTION){
        System.exit(1);
      }
      File fil = velger.getSelectedFile();
      

      //Oppretter labyrinten
      Labyrint l = null;
      try{
        l = new Labyrint(fil);
      }
      catch(FileNotFoundException e){
        System.exit(1);
      }

      //Viser Labyrinten grafisk med swing
      l.grafisk();

  }
}
