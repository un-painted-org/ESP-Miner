import { HttpErrorResponse } from '@angular/common/http';
import { Component, Input, OnInit, OnDestroy } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { ToastrService } from 'ngx-toastr';
import { forkJoin, startWith, Subject, takeUntil } from 'rxjs';
import { LoadingService } from 'src/app/services/loading.service';
import { SystemService } from 'src/app/services/system.service';
import { eASICModel } from 'src/models/enum/eASICModel';
import { ActivatedRoute } from '@angular/router';

@Component({
  selector: 'app-edit',
  templateUrl: './edit.component.html',
  styleUrls: ['./edit.component.scss']
})
export class EditComponent implements OnInit, OnDestroy {

  public form!: FormGroup;

  public firmwareUpdateProgress: number | null = null;
  public websiteUpdateProgress: number | null = null;

  public savedChanges: boolean = false;
  public settingsUnlocked: boolean = false;
  public eASICModel = eASICModel;
  public ASICModel!: eASICModel;
  public restrictedModels: eASICModel[] = Object.values(eASICModel)
    .filter((v): v is eASICModel => typeof v === 'string');

  @Input() uri = '';

  // Store frequency and voltage options from API
  public frequencyOptions: number[] = [];
  public voltageOptions: number[] = [];

  public ticketMaskDiffOptions: number[] = [];
  public versionMaskOptions: number[] = [];

  // Default values for different ASIC models
  private defaultFrequencies: Record<eASICModel, number> = {
    [eASICModel.BM1366]: 485,
    [eASICModel.BM1368]: 490,
    [eASICModel.BM1370]: 525,
    [eASICModel.BM1397]: 425
  };

  private defaultVoltages: Record<eASICModel, number> = {
    [eASICModel.BM1366]: 1200,
    [eASICModel.BM1368]: 1166,
    [eASICModel.BM1370]: 1150,
    [eASICModel.BM1397]: 1400
  };

  private defaultTicketMaskDiff: number = 256;
  private defaultVersionMask: number = 536862720;

  private destroy$ = new Subject<void>();

  constructor(
    private fb: FormBuilder,
    private systemService: SystemService,
    private toastr: ToastrService,
    private loadingService: LoadingService,
    private route: ActivatedRoute,
  ) {
    // Check URL parameter for settings unlock
    this.route.queryParams.subscribe(params => {
      const urlOcParam = params['oc'] !== undefined;
      if (urlOcParam) {
        // If ?oc is in URL, enable overclock and save to NVS
        this.settingsUnlocked = true;
        this.saveOverclockSetting(1);
        console.log(
          '🎉 The ancient seals have been broken!\n' +
          '⚡ Unlimited power flows through your miner...\n' +
          '🔧 You can now set custom frequency and voltage values.\n' +
          '⚠️ Remember: with great power comes great responsibility!'
        );
      } else {
        // If ?oc is not in URL, check NVS setting (will be loaded in ngOnInit)
        console.log('🔒 Here be dragons! Advanced settings are locked for your protection. \n' +
          'Only the bravest miners dare to venture forth... \n' +
          'If you wish to unlock dangerous overclocking powers, add: %c?oc',
          'color: #ff4400; text-decoration: underline; cursor: pointer; font-weight: bold;',
          'to the current URL'
        );
      }
    });
  }

  private saveOverclockSetting(enabled: number) {
    this.systemService.updateSystem(this.uri, { overclockEnabled: enabled })
      .subscribe({
        next: () => {
          console.log(`Overclock setting saved: ${enabled === 1 ? 'enabled' : 'disabled'}`);
        },
        error: (err) => {
          console.error(`Failed to save overclock setting: ${err.message}`);
        }
      });
  }

  ngOnInit(): void {
    // Fetch both system info and ASIC settings in parallel
    forkJoin({
      info: this.systemService.getInfo(this.uri),
      asicSettings: this.systemService.getAsicSettings(this.uri),
      asicMaskSettings: this.systemService.getAsicMaskSettings(this.uri)
    })
    .pipe(
      this.loadingService.lockUIUntilComplete(),
      takeUntil(this.destroy$)
    )
    .subscribe(({ info, asicSettings, asicMaskSettings }) => {
      this.ASICModel = info.ASICModel;

      // Store the frequency and voltage options from the API
      this.frequencyOptions = asicSettings.frequencyOptions;
      this.voltageOptions = asicSettings.voltageOptions;

      this.ticketMaskDiffOptions = asicMaskSettings.ticketMaskDiffOptions;
      this.versionMaskOptions = asicMaskSettings.versionMaskOptions;

      // Check if overclock is enabled in NVS
      if (info.overclockEnabled === 1) {
        this.settingsUnlocked = true;
        console.log(
          '🎉 Overclock mode is enabled from NVS settings!\n' +
          '⚡ Custom frequency and voltage values are available.'
        );
      }

        this.form = this.fb.group({
          flipscreen: [info.flipscreen == 1],
          invertscreen: [info.invertscreen == 1],
          displayTimeout: [info.displayTimeout, [
            Validators.required,
            Validators.pattern(/^[^:]*$/),
            Validators.min(-1),
            Validators.max(71582)
          ]],
          coreVoltage: [info.coreVoltage, [Validators.required]],
          frequency: [info.frequency, [Validators.required]],
          ticketMaskDiff: [info.ticketMaskDiff, [Validators.required]],
          versionMask: [info.versionMask, [Validators.required]],
          autofanspeed: [info.autofanspeed == 1, [Validators.required]],
          fanspeed: [info.fanspeed, [Validators.required]],
          temptarget: [info.temptarget, [Validators.required]],
          overheat_mode: [info.overheat_mode, [Validators.required]]
        });

      this.form.controls['autofanspeed'].valueChanges.pipe(
        startWith(this.form.controls['autofanspeed'].value),
        takeUntil(this.destroy$)
      ).subscribe(autofanspeed => {
        if (autofanspeed) {
          this.form.controls['fanspeed'].disable();
          this.form.controls['temptarget'].enable();
        } else {
          this.form.controls['fanspeed'].enable();
          this.form.controls['temptarget'].disable();
        }
      });
    });
  }

  ngOnDestroy(): void {
    this.destroy$.next();
    this.destroy$.complete();
  }

  public updateSystem() {

    const form = this.form.getRawValue();

    if (form.stratumPassword === '*****') {
      delete form.stratumPassword;
    }

    this.systemService.updateSystem(this.uri, form)
      .pipe(this.loadingService.lockUIUntilComplete())
      .subscribe({
        next: () => {
          const successMessage = this.uri ? `Saved settings for ${this.uri}` : 'Saved settings';
          this.toastr.success(successMessage, 'Success!');
          this.savedChanges = true;
        },
        error: (err: HttpErrorResponse) => {
          const errorMessage = this.uri ? `Could not save settings for ${this.uri}. ${err.message}` : `Could not save settings. ${err.message}`;
          this.toastr.error(errorMessage, 'Error');
          this.savedChanges = false;
        }
      });
  }

  showWifiPassword: boolean = false;
  toggleWifiPasswordVisibility() {
    this.showWifiPassword = !this.showWifiPassword;
  }

  disableOverheatMode() {
    this.form.patchValue({ overheat_mode: 0 });
    this.updateSystem();
  }

  toggleOverclockMode(enable: boolean) {
    this.settingsUnlocked = enable;
    this.saveOverclockSetting(enable ? 1 : 0);

    if (enable) {
      console.log(
        '🎉 Overclock mode enabled!\n' +
        '⚡ Custom frequency and voltage values are now available.'
      );
    } else {
      console.log('🔒 Overclock mode disabled. Using safe preset values only.');
    }
  }

  public restart() {
    this.systemService.restart(this.uri)
      .pipe(this.loadingService.lockUIUntilComplete())
      .subscribe({
        next: () => {
          const successMessage = this.uri ? `Bitaxe at ${this.uri} restarted` : 'Bitaxe restarted';
          this.toastr.success(successMessage, 'Success');
        },
        error: (err: HttpErrorResponse) => {
          const errorMessage = this.uri ? `Failed to restart device at ${this.uri}. ${err.message}` : `Failed to restart device. ${err.message}`;
          this.toastr.error(errorMessage, 'Error');
        }
      });
  }

  getDropdownFrequency() {
    if (!this.frequencyOptions.length) {
      return [];
    }

    // Convert frequency options from API to dropdown format
    const options = this.frequencyOptions.map(freq => {
      // Check if this is a default frequency for the current ASIC model
      const isDefault = this.defaultFrequencies[this.ASICModel] === freq;
      return {
        name: isDefault ? `${freq} (default)` : `${freq}`,
        value: freq
      };
    });

    // Get current frequency value from form
    const currentFreq = this.form?.get('frequency')?.value;

    // If current frequency exists and isn't in the options
    if (currentFreq && !options.some(opt => opt.value === currentFreq)) {
      options.push({
        name: `${currentFreq} (Custom)`,
        value: currentFreq
      });
      // Sort options by frequency value
      options.sort((a, b) => a.value - b.value);
    }

    return options;
  }

  getCoreVoltage() {
    if (!this.voltageOptions.length) {
      return [];
    }

    // Convert voltage options from API to dropdown format
    const options = this.voltageOptions.map(voltage => {
      // Check if this is a default voltage for the current ASIC model
      const isDefault = this.defaultVoltages[this.ASICModel] === voltage;
      return {
        name: isDefault ? `${voltage} (default)` : `${voltage}`,
        value: voltage
      };
    });

    // Get current voltage value from form
    const currentVoltage = this.form?.get('coreVoltage')?.value;

    // If current voltage exists and isn't in the options
    if (currentVoltage && !options.some(opt => opt.value === currentVoltage)) {
      options.push({
        name: `${currentVoltage} (Custom)`,
        value: currentVoltage
      });
      // Sort options by voltage value
      options.sort((a, b) => a.value - b.value);
    }

    return options;
  }

  getStratumDiff() {
    if (!this.ticketMaskDiffOptions.length) {
      return [];
    }

    // Convert ticket maks diff options from API to dropdown format
    const options = this.ticketMaskDiffOptions.map(ticketMaskDiff => {
      // Check if this is a default ticket maks diff 
      const isDefault = this.defaultTicketMaskDiff === ticketMaskDiff;
      return {
        name: isDefault ? `${ticketMaskDiff} (default)` : `${ticketMaskDiff}`,
        value: ticketMaskDiff
      };
    });

    // Get current ticket maks diff value from form
    const currentTicketMaskDiff = this.form?.get('stratumDiff')?.value;

    // If current ticket maks diff exists and isn't in the options
    if (currentTicketMaskDiff && !options.some(opt => opt.value === currentTicketMaskDiff)) {
      options.push({
        name: `${currentTicketMaskDiff} (Custom)`,
        value: currentTicketMaskDiff
      });
      // Sort options by ticket maks diff value
      options.sort((a, b) => a.value - b.value);
    }

    return options;
  }

  getVersionMask() {
    if (!this.versionMaskOptions.length) {
      return [];
    }

    // Convert version mask options from API to dropdown format
    const options = this.versionMaskOptions.map(versionMask => {
      // Check if this is a default ticket maks diff 
      const isDefault = this.defaultVersionMask === versionMask;
      const hexValue = versionMask.toString(16); // Umwandlung in hexadezimale Form
      return {
        name: isDefault ? `${versionMask} - 0x${hexValue} (default)` : `${versionMask} - 0x${hexValue}`,
        value: versionMask
      };
    });

    // Get current version mask value from form
    const currentVersionMask = this.form?.get('versionMask')?.value;

    // If current version mask exists and isn't in the options
    if (currentVersionMask && !options.some(opt => opt.value === currentVersionMask)) {
      const hexCurrentValue = currentVersionMask.toString(16); // Umwandlung in hexadezimale Form
      options.push({
        name: `${currentVersionMask} - 0x${hexCurrentValue} (Custom)`,
        value: currentVersionMask
      });
      // Sort options by version mask  value
      options.sort((a, b) => a.value - b.value);
    }

    return options;
  }
}
