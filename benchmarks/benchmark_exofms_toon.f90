! Benchmark the Exo-FMS Toon shortwave and longwave solvers for independent
! atmospheric columns.  The solver modules are compiled from a separately
! checked-out Exo-FMS source tree; they are not vendored here.
program benchmark_exofms_toon
  use, intrinsic :: iso_fortran_env, only : real64
  use omp_lib, only : omp_get_max_threads, omp_get_wtime
  use WENO4_mod, only : interpolate_weno4
  use sw_Toon_mod, only : sw_Toon
  use lw_Toon_mod, only : lw_Toon
  implicit none

  integer, parameter :: dp = real64
  integer :: nprofile, nlay, nlev, warmup, repeats
  integer :: profile, repeat, level, omp_threads
  real(dp) :: sw_seconds, lw_seconds, checksum, wall_start
  real(dp) :: f_inc, sw_albedo, lw_albedo, tint, contribution, ssa_value, asymmetry_value
  real(dp), allocatable :: tau_edge(:, :), mu_z(:, :), ssa(:, :), asymmetry(:, :)
  real(dp), allocatable :: temperature(:, :), pressure_layer(:, :), pressure_edge(:, :)
  real(dp), allocatable :: temperature_edge(:, :)
  real(dp), allocatable :: sw_up(:, :), sw_down(:, :), sw_net(:, :)
  real(dp), allocatable :: lw_up(:, :), lw_down(:, :), lw_net(:, :)
  character(len=32) :: argument
  logical :: print_accuracy

  call require_integer_argument(1, nprofile, 'nprofile')
  call optional_integer_argument(2, 40, nlay)
  call optional_integer_argument(3, 1, warmup)
  call optional_integer_argument(4, 3, repeats)
  call optional_real_argument(5, 0.5_dp, ssa_value)
  call optional_real_argument(6, 0.5_dp, asymmetry_value)
  if (nprofile < 1 .or. nlay < 11 .or. nlay > 150 .or. warmup < 0 .or. repeats < 1) then
    error stop 'usage: benchmark_exofms_toon nprofile [nlay=40] [warmup=1] [repeats=3] [ssa=0.5] [g=0.5]'
  end if

  nlev = nlay + 1
  allocate(tau_edge(nlev, nprofile), mu_z(nlev, nprofile), ssa(nlay, nprofile), asymmetry(nlay, nprofile))
  allocate(temperature(nlay, nprofile), pressure_layer(nlay, nprofile), pressure_edge(nlev, nprofile))
  allocate(temperature_edge(nlev, nprofile))
  allocate(sw_up(nlev, nprofile), sw_down(nlev, nprofile), sw_net(nlev, nprofile))
  allocate(lw_up(nlev, nprofile), lw_down(nlev, nprofile), lw_net(nlev, nprofile))

  do profile = 1, nprofile
    do level = 1, nlev
      tau_edge(level, profile) = 0.1_dp * real(level - 1, dp)
      mu_z(level, profile) = 0.5_dp
      pressure_edge(level, profile) = 1.0e2_dp * (1.0e3_dp ** (real(level - 1, dp) / real(nlay, dp)))
    end do
    do level = 1, nlay
      pressure_layer(level, profile) = sqrt(pressure_edge(level, profile) * pressure_edge(level + 1, profile))
      if (level <= 5) then
        temperature(level, profile) = 1.0_dp
      else if (level >= nlay - 4) then
        temperature(level, profile) = 300.0_dp
      else
        temperature(level, profile) = 1.0_dp + 299.0_dp * real(level - 5, dp) / real(nlay - 10, dp)
      end if
    end do
    temperature_edge(:, profile) = interpolate_weno4(pressure_edge(:, profile), pressure_layer(:, profile), &
      temperature(:, profile), .False.)
    temperature_edge(1, profile) = 10.0_dp**(log10(temperature(1, profile)) + &
      (log10(pressure_edge(1, profile) / pressure_edge(2, profile)) / &
      log10(pressure_layer(1, profile) / pressure_edge(2, profile))) * &
      log10(temperature(1, profile) / temperature_edge(2, profile)))
    temperature_edge(nlev, profile) = 10.0_dp**(log10(temperature(nlay, profile)) + &
      (log10(pressure_edge(nlev, profile) / pressure_layer(nlay, profile)) / &
      log10(pressure_layer(nlay, profile) / pressure_edge(nlay, profile))) * &
      log10(temperature(nlay, profile) / temperature_edge(nlev - 1, profile)))
  end do
  ssa = ssa_value
  asymmetry = asymmetry_value
  f_inc = 1.0_dp
  sw_albedo = 0.1_dp
  lw_albedo = 0.0_dp
  tint = 0.0_dp
  argument = ''
  call get_environment_variable('EXOFMS_PRINT_ACCURACY', argument)
  print_accuracy = len_trim(argument) > 0 .and. trim(argument) /= '0'
  omp_threads = omp_get_max_threads()

  checksum = 0.0_dp
  do repeat = 1, warmup
!$omp parallel do reduction(+:checksum) private(contribution) schedule(static)
    do profile = 1, nprofile
      call sw_contribution(nlay, nlev, tau_edge(:, profile), mu_z(:, profile), f_inc, ssa(:, profile), &
                           asymmetry(:, profile), sw_albedo, sw_up(:, profile), sw_down(:, profile), &
                           sw_net(:, profile), contribution)
      checksum = checksum + contribution
      call lw_contribution(nlay, nlev, temperature(:, profile), pressure_layer(:, profile), &
                           pressure_edge(:, profile), tau_edge(:, profile), ssa(:, profile), &
                           asymmetry(:, profile), lw_albedo, tint, lw_up(:, profile), lw_down(:, profile), &
                           lw_net(:, profile), contribution)
      checksum = checksum + contribution
    end do
!$omp end parallel do
  end do

  wall_start = omp_get_wtime()
  do repeat = 1, repeats
!$omp parallel do reduction(+:checksum) private(contribution) schedule(static)
    do profile = 1, nprofile
      call sw_contribution(nlay, nlev, tau_edge(:, profile), mu_z(:, profile), f_inc, ssa(:, profile), &
                           asymmetry(:, profile), sw_albedo, sw_up(:, profile), sw_down(:, profile), &
                           sw_net(:, profile), contribution)
      checksum = checksum + contribution
    end do
!$omp end parallel do
  end do
  sw_seconds = (omp_get_wtime() - wall_start) / real(repeats, dp)

  wall_start = omp_get_wtime()
  do repeat = 1, repeats
!$omp parallel do reduction(+:checksum) private(contribution) schedule(static)
    do profile = 1, nprofile
      call lw_contribution(nlay, nlev, temperature(:, profile), pressure_layer(:, profile), &
                           pressure_edge(:, profile), tau_edge(:, profile), ssa(:, profile), &
                           asymmetry(:, profile), lw_albedo, tint, lw_up(:, profile), lw_down(:, profile), &
                           lw_net(:, profile), contribution)
      checksum = checksum + contribution
    end do
!$omp end parallel do
  end do
  lw_seconds = (omp_get_wtime() - wall_start) / real(repeats, dp)

  write(*, '(a,i0,a,i0,a,i0,a,i0)') 'nprofile=', nprofile, ',nlay=', nlay, &
    ',warmup=', warmup, ',repeats=', repeats
  write(*, '(a,i0)') 'openmp_threads=', omp_threads
  write(*, '(a,f0.9)') 'exofms_sw_toon_seconds=', sw_seconds
  write(*, '(a,f0.9)') 'exofms_lw_toon_5node_seconds=', lw_seconds
  write(*, '(a,es24.16)') 'checksum=', checksum
  if (print_accuracy) then
    do level = 1, nlev
      write(*, '(a,i0,a,es24.16,a,es24.16,a,es24.16,a,es24.16,a,es24.16)') &
        'accuracy_level=', level, ',temperature=', temperature_edge(level, 1), &
        ',sw_up=', sw_up(level, 1), ',sw_down=', sw_down(level, 1), &
        ',lw_up=', lw_up(level, 1), ',lw_down=', lw_down(level, 1)
    end do
  end if

contains

  subroutine require_integer_argument(position, value, name)
    integer, intent(in) :: position
    integer, intent(out) :: value
    character(len=*), intent(in) :: name

    if (command_argument_count() < position) error stop 'missing '//name
    call get_command_argument(position, argument)
    read(argument, *) value
  end subroutine require_integer_argument

  subroutine optional_integer_argument(position, default_value, value)
    integer, intent(in) :: position, default_value
    integer, intent(out) :: value

    value = default_value
    if (command_argument_count() >= position) then
      call get_command_argument(position, argument)
      read(argument, *) value
    end if
  end subroutine optional_integer_argument

  subroutine optional_real_argument(position, default_value, value)
    integer, intent(in) :: position
    real(dp), intent(in) :: default_value
    real(dp), intent(out) :: value

    value = default_value
    if (command_argument_count() >= position) then
      call get_command_argument(position, argument)
      read(argument, *) value
    end if
  end subroutine optional_real_argument

  subroutine sw_contribution(nlay, nlev, tau_edge, mu_z, f_inc, ssa, asymmetry, albedo, &
                             sw_up, sw_down, sw_net, result)
    integer, intent(in) :: nlay, nlev
    real(dp), intent(in) :: tau_edge(nlev), mu_z(nlev), f_inc, ssa(nlay), asymmetry(nlay), albedo
    real(dp), intent(out) :: sw_up(nlev), sw_down(nlev), sw_net(nlev), result
    real(dp) :: mu_z_local(nlev), asr

    mu_z_local = mu_z
    call sw_Toon(nlay, nlev, tau_edge, mu_z_local, f_inc, ssa, asymmetry, albedo, &
                 sw_up, sw_down, sw_net, asr)
    result = sw_net(nlev) + asr
  end subroutine sw_contribution

  subroutine lw_contribution(nlay, nlev, temperature, pressure_layer, pressure_edge, tau_edge, &
                             ssa, asymmetry, albedo, tint, lw_up, lw_down, lw_net, result)
    integer, intent(in) :: nlay, nlev
    real(dp), intent(in) :: temperature(nlay), pressure_layer(nlay), pressure_edge(nlev), tau_edge(nlev)
    real(dp), intent(in) :: ssa(nlay), asymmetry(nlay), albedo, tint
    real(dp), intent(out) :: lw_up(nlev), lw_down(nlev), lw_net(nlev), result
    real(dp) :: olr

    call lw_Toon(nlay, nlev, temperature, pressure_layer, pressure_edge, tau_edge, ssa, asymmetry, &
                 albedo, tint, lw_up, lw_down, lw_net, olr)
    result = lw_net(1) + olr
  end subroutine lw_contribution

end program benchmark_exofms_toon
